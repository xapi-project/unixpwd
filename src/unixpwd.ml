
module Stubs = struct
    external get    :   string -> string          = "caml_unixpwd_getpwd"
    external setpwd :   string -> string -> unit  = "caml_unixpwd_setpwd"
    external setspw :   string -> string -> unit  = "caml_unixpwd_setspw"
    external unshadow : unit   -> string          = "caml_unixpwd_unshadow"
end
