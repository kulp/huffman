\ vi:set syntax=forth ts=2 sw=2 et: \

: emit-num ( n -- ) \ emits a number with no space following
  s>d 0 d.r ;

: emit-bit-at ( n1 n2 -- ) \ emits bit from n1 at position n2
  1 swap lshift over and 0= invert negate emit-num ;

: verilog-num ( number len -- ) \ prints number in Verilog fmt
  dup emit-num ." 'b" \ emit bit len, radix
  1- 0 swap do i emit-bit-at -1 +loop drop ;

: print-hex-num ( val -- )
  base @  hex swap ." 0x" .  base ! ;

: print-tree-entry ( val len key -- )
  print-hex-num ." =" space verilog-num ;

: 3dup ( x1 x2 x3 -- x1 x2 x3 x1 x2 x3 )
  dup 2over rot ;

: set-up-child ( val2 len2 addr2 inc bit -- val2 len2 addr2 )
  >r + rot  1 lshift r> or rot  1+ rot ;

: emit-node ( val len addr -- )
  dup c@ over 1+ c@ \ get two bytes
  dup 0= if
    2drop c@ print-tree-entry cr
  else
    2>r 3dup
    r> 1 set-up-child recurse
    r> 0 set-up-child recurse
  then ;

: unmake-dict ( fid -- ) \ slurps fid
  slurp-fid drop 0 0 rot emit-node ;

