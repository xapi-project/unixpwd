
let user = "unixpwd"

let cycle n =
  let pw = Printf.sprintf "unixpwd-%06d" n in
  Unixpwd.setspw user pw;
  ignore (Unixpwd.unshadow () |> String.length);
  assert (Unixpwd.getspw user = pw);
  Unixpwd.setpwd user pw;
  assert (Unixpwd.getpwd user = pw);
  if n mod 10 = 0 then Printf.eprintf "cycle %6d\n" n

let main () =
  for i = 1 to 100_000 do
    cycle i
  done

let () =
  if !Sys.interactive then
    ()
  else
    try
      main ()
    with
      e ->
      Printf.eprintf "error: %s\n" (Printexc.to_string e);
      exit 1

