cd /d "%~dp0..\.."
start x64\Debug\WinXShell.exe -console -ui -jcfg wxsUI\UI_SystemInfo\main.jcfg
pause
start x64\Debug\WinXShell.exe -console -ui -jcfg wxsUI\UI_SystemInfo\main.jcfg -theme dark
