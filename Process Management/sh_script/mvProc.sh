if find ../process_c_out_submitted -mindepth 1 -print -quit | grep -q .; then
	mv ../process_c_out_submitted/*.out ../process_c_out_ex
fi
