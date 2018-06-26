
let user = "unixpwd"

let cycle n =
  let pw = Printf.sprintf "unixpwd-%06d" n in
  Unixpwd.Stubs.setspw user pw;
  ignore (Unixpwd.Stubs.unshadow () |> String.length);
  assert (Unixpwd.Stubs.get user = pw);
  Unixpwd.Stubs.setpwd user pw;
  assert (Unixpwd.Stubs.get user = pw)

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

