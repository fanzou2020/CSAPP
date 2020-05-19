# CSAPP: Data Lab

### bitXor
`p ^ q = (~p & ~q) | (p & ~q)`       
`p | q = ~(~p & ~q)`


### tmin
`1 << 31`, will get the Minimum integer.

### isTmax
`tmp = (Tmax << 1) + 1 = 0xFFFFFFFF`    
`!(~tmp)` will return `1`, if `x` i `Tmax`. 
