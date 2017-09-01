\ vi:set syntax=forth ts=2 sw=2 et: \

: emit-num ( n -- ) \ emits a number with no space following
  s>d 0 d.r ;

: emit-bit-at ( n1 n2 -- ) \ emits bit from n1 at position n2
  1 swap lshift and 0<> negate emit-num ;

: verilog-num ( number len -- ) \ prints number in Verilog fmt
  dup emit-num ." 'b" \ emit bit len, radix
  1- 0 swap do dup i emit-bit-at -1 +loop drop ;

: print-hex-num ( val -- )
  base @  hex swap ." 0x" .  base ! ;

: print-tree-entry ( val len key -- )
  print-hex-num ." =" space verilog-num cr ;

: 4dup ( x1 .. x4 -- x1 .. x4 x1 .. x4 )
  2over 2over ;

: set-up-child ( val1 len1 addr1 inc bit -- val2 len2 addr2 )
  >r + rot  1 lshift r> or rot  1+ rot ;

: proc-node ( xt val len addr -- )
  dup c@ over 1+ c@ \ get two bytes
  dup 0= if
    2drop c@ 3 pick execute drop
  else
    2>r 4dup
    r> 1 set-up-child recurse
    r> 0 set-up-child recurse
  then ;

: unmake-dict ( fid -- ) \ slurps fid
  slurp-fid drop ['] print-tree-entry swap 0 0 rot proc-node ;

