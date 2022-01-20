# NeoCalc

A small calculator with variables.

---

## Building

### Requied Libraries

#### libreadline
**Used For**
- REPL

#### [z_](https://github.com/zakarouf/z_)
**Used For**
  - String
  - Variables
  - Stack System

After fetching all the libraries run
```
sh build.sh
```

This will create a binary `nc`.

## Using NeoCalc

```sh
./nc
```
> Will Initiate the REPL

```racket
$ ./nc
Welcome to neocalc (https://github.com/zakarouf/neocalc)
neocalc 0.1.0 by zakarouf 2022-2023 (/q to exit)
> 13
stdout:13.000000
```
Type any number to push to the default `stdout`.

### Evaluation

Expression are written in lisp like S-Expression.
```racket
> (+ 1 2 3)
stdout:6.000000
```

They can be nested too!
```racket
> (- 40 (* 12 4))
stdout:-8.000000
> (- 40 (* 12 4) (/ 22 7) (+ 24 (- 20 77)))
stdout:21.857143
```

### Variables

You can store a result of an expression inside a variable using `/s` command
```racket
> /s x (+ 1 2 3)
x:6.000000
```

You can call that variable again inside an expression to use its value again.
```racket
> (* 2 x)
stdout:12.000000
```

