if find ../process_c_out_submitted -mindepth 1 -print -quit | grep -q .; then
	set `ls ../process_c_out_submitted`
	retVal=''
	space=' '
	while(true)
	do
		if [ -n "$1" ]; then
			retVal=$retVal$space$1
		fi
	
		if [ -n "$1" ]; then
			shift 1
		else
			break
		fi
	done
	echo $retVal
else
	echo "NULL"
fi

