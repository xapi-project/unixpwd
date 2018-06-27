
module Stubs = struct
    external getpwd :   string -> string          = "caml_unixpwd_getpwd"
    external getspw :   string -> string          = "caml_unixpwd_getspw"
    external get    :   string -> string          = "caml_unixpwd_get"
    external setpwd :   string -> string -> unit  = "caml_unixpwd_setpwd"
    external setspw :   string -> string -> unit  = "caml_unixpwd_setspw"
    external unshadow : unit   -> string          = "caml_unixpwd_unshadow"
end
