<div align="center">
  <img src="docs/neocalc0.2.1.png" width="600"/>
  <h1>neocalc</h1>

  A small lisp-like calculator with variables.
</div>

## Highlights

- Calculator
- Lisp-like evaluation
- Uses Stack System; no linked list
- Easy to implement your own operators (see `list_op` function in main.c)

> neocalc is an evolution of the [calc](https://github.com/zakarouf/simple_calc) my first ever program.

---

## Building

### Requied Libraries

- **libreadline**: For REPL

- [z_](https://github.com/zakarouf/z_)
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
_ is 13.000000
```
Type any number to push to the default variable `_`.

### Evaluation

Expression are written in S-Expression like syntax.
```racket
> (+ 1 2 3)
_ is 6.000000
```

They can be nested too!
```racket
> (- 40 (* 12 4))
_ is -8.000000
> (- 40 (* 12 4) (/ 22 7) (+ 24 (- 20 77)))
stdout:21.857143
```

### Defining Variables

You can store a result of an expression inside a variable using `set` command
```racket
> (set x (+ 1 2 3))
> /v x
x is 6.000000
```
> NOTE: `/v` is a repl command that sets a result variable

You can call that variable again inside an expression to use its value again.
```racket
> (* 2 x)
x is 12.000000
```

### Defining Expression

One can define a expression like so
```racket
> (set @sqr (* #0 #0))
> (@sqr 3)
_ is 9.000000
```
> `#[0-9]+` is used to define a passed argument

### Loading a file

It is a hassle to set the same boilerplate symbols and values over and over
again. Instead we can write all our definations and load it in when we need.
```racket
# util.txt
(set @snd (+ (* 2 #0) 1))
(set @sqr (* #0 #0))
(set @sa (@sqr (@snd #0)))
```
> NOTE: Repl specific commands, i.e. `/v`, '/q' etc. are not valid here

We load it up in our relp, like so
```racket
> (load util.txt)
> (@sa 0)
_ is 1.000000
> (@sa 1)
_ is 9.000000
```

