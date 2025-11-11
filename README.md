# Lispa

A simple lisp programming language.

### Compile Comand on Windows
``` sh
cc -std=c99 -Wall lispa.c mpc.c -o lispa
```
### Compile Comand on Linux / Mac

``` sh
cc -std=c99 -Wall lispa.c mpc.c -ledit -lm -o lispa
```

## Examples
#### Coments
```
; use a semicolon for single-line comments
```
#### Variable definitions
Define variable x as 100 and message as "hello world"
(def {x} 100)
(def {message} "hello world")
```
; prints "hello world"
(print message)
; prints x + 3
(print (+ x 3))
```
#### If statements
```
; boolean condition checking that x is greater than 10
(if (> x 10) 
    ; if x is greater than 10
    {print "x is greater than 10"} 
    ; otherwise
    {print "x is not greater than 10"}
)
; true boolean condition
(if (true)
    {print "is true"}
    {print "is false"}
)
```
#### List methods
```
; Apply the function - to a list
(print (map - {5 6 7 8 2 22 44}) )

; Filter all numbers greater than 2
(print (filter (\ {x} {> x 2}) {5 2 11 -7 8 1}))
```
#### Switch statement
```
; Switch case for day name
(func {day-name x} {
  case x
    {0 "Monday"}
    {1 "Tuesday"}
    {2 "Wednesday"}
    {3 "Thursday"}
    {4 "Friday"}
    {5 "Saturday"}
    {6 "Sunday"}
})
(print (day-name 2))

(print (day-name 2))
```



