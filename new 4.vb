#$language = "VBScript"
#$interface = "1.0"
Sub main
	Dir = "d:\baohua"
	'turn on synchronous = True
	crt.Screen.Synchronous = True
	crt.Screen.WaitForString = "CCC"
	crt.F ileTransfer.SendXmodem Dir & "f ile.bin" 
	'wait "y/n" string then send "y"
	crt.Screen.WaitForString "y/n" 
	crt.Screen.Send "y" & VbCr
	
End Sub