(*
 * Copyright (c) 1997-1999 Massachusetts Institute of Technology
 * Copyright (c) 2003, 2007-8 Matteo Frigo
 * Copyright (c) 2003, 2007-8 Massachusetts Institute of Technology
 *
 * See the file COPYING for license information.
 *
 *)

(*************************************************************
 *    Functional associative table
 *************************************************************)

(* 
 * this module implements a functional associative table.  
 * The table is parametrized by an equality predicate and
 * a hash function, with the restriction that (equal a b) ==>
 * hash a == hash b.
 * The table is purely functional and implemented using a binary
 * search tree (not balanced for now)
 *)

type ('a, 'b) elem = 
    Leaf 
  | Node of int * ('a, 'b) elem * ('a, 'b) elem * ('a * 'b) list

let empty = Leaf

let lookup hash equal key table =
  let h = hash key in
  let rec look = function
      Leaf -> None
    | Node (hash_key, left, right, this_list) ->
        if (hash_key < h) then look left
        else if (hash_key > h) then look right
        else let rec loop = function
            [] -> None
          | (a, b) :: rest -> if (equal key a) then Some b else loop rest
        in loop this_list
  in look table

let insert hash key value table =
  let h = hash key in
  let rec ins = function
      Leaf -> Node (h, Leaf, Leaf, [(key, value)])
    | Node (hash_key, left, right, this_list) ->
        if (hash_key < h) then 
          Node (hash_key, ins left, right, this_list)
        else if (hash_key > h) then 
          Node (hash_key, left, ins right, this_list)
        else 
          Node (hash_key, left, right, (key, value) :: this_list)
  in ins table