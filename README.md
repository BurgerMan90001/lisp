# Lispa

A simple lisp programming language.

## Compiling and Running

Compile commands

### On Windows

```sh
cc -std=c99 -Wall lispa.c mpc.c -o lispa
```

### On Linux / Mac

```sh
cc -std=c99 -Wall lispa.c mpc.c -ledit -lm -o lispa
```

Then run lispa in the same file directory

### Run using the command line interface

```sh
./lispa
```

### Run a file

```sh
./lispa ./filename
```

## Examples

#### Comments

```
; use a semicolon for single-line comments
```

#### Variable definitions

Define variable x as 100 and message as "hello world"

```
(def {x} 100)
(def {message} "hello world")
```

#### Printing

Prints "hello world" and x + 3 which is 103.

```
(def {message} "hello world")
(def {x} 100)

(print message)
(print (+ x 3))
```

#### If statements

Boolean condition checking that x is greater than 10.
There are also true and false boolean conditions.

```
(if (> x 10)
    ; if x is greater than 10
    {print "x is greater than 10"}
    ; otherwise
    {print "x is not greater than 10"}
)
(if (true)
    {print "is true"}
    {print "is false"}
)
```

#### User-defined Functions

Defines a function named get_sum with x and y as arguements

```
(func {get_sum x y}
    {+ x y}
)

(print (get_sum 3 4))
```

#### List methods

Map applying the function - to a list.
Filter all numbers greater than 2.

```
(print (map - {5 6 7 8 2 22 44}) )

(print (filter (\ {x} {> x 2}) {5 2 11 -7 8 1}))
```

#### Switch statement

Switch statement for day name from number

```
(func {day_name x} {
  case x
    {0 "Monday"}
    {1 "Tuesday"}
    {2 "Wednesday"}
    {3 "Thursday"}
    {4 "Friday"}
    {5 "Saturday"}
    {6 "Sunday"}
})

(print (day_name 2))
```
