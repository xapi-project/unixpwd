
let main () =
    Unixpwd.Stubs.unshadow () |> print_endline

let () = if !Sys.interactive then () else main ()

