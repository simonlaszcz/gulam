setenv PATH c:\bin,d:\bin
setenv TEMP f:\
alias m more
alias hi history
alias lo exit
alias cc c:\bin\cc.prg
alias x ue
alias xm c:\bin\xmdm.tos
alias rm rm -i
alias zm c:\bin\zmdm.prg
alias cf d:\bin\compress -v
alias uc d:\bin\compress -vd
alias ls 'ls -F'
alias cf 'd:\bin\compress.ttp -v'
alias uc 'd:\bin\compress.ttp -vd'
alias mwc e:\profile.g	#set up for mark williams
cd c:\home
set home $cwd
set prompt '$cwd '
set prompt_tail	' # '
set mscursor '0000'
set baud_rate 9600
set sz_rs232_buffer 4096
set histfile c:\history.g
set history 25
set restore_cwd_after_exec 1
rehash
