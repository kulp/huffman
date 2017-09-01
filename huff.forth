\ vi:set syntax=forth ts=2 sw=2 et: \

: emit-num ( n -- ) \ emits a number with no space following
  s>d 0 d.r ;

: verilog-num ( number len -- ) \ prints number in Verilog fmt
  dup emit-num [char] ' emit [char] b emit \ emit bit len, radix
  1- 0 swap do
    1 i lshift over and 0= invert negate emit-num
  -1 +loop drop ;

: print-tree-entry ( val len key -- )
  base @  hex swap        \ save base, switch to hex
  [char] 0 emit
  [char] x emit
  .
  [char] = emit
  base !                  \ restore base
  space  verilog-num ;

: 3dup ( x1 x2 x3 -- x1 x2 x3 x1 x2 x3 )
  dup 2>r 2dup r> rot rot r> ;

: emit-node ( val len addr -- )
  dup c@ over 1+ c@     ( val len addr x1 x2 )
  dup 0=                ( val len addr x1 x2 f )
    if
      2drop c@ print-tree-entry cr
    else
      2>r \ save x2 x1
      3dup \ duplicate state
      r> + rot \ addr + x1
      1 lshift 1 or rot \ (val << 1) | 1
      1+ rot \ len + 1
      recurse
      r> + rot \ addr + x2
      1 lshift rot \ val << 1
      1+ rot \ len + 1
      recurse
    then
  ;

: unmake-dict ( fid -- ) \ slurps fid
  slurp-fid drop 0 0 rot emit-node ;

