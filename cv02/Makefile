cv2_arping: 
	gcc -Wno-address-of-packed-member -Wall -I./src -I../  src/cv2_arping.c -o cv2_arping
	sudo setcap cap_net_raw+eip cv2_arping
	
clean:
	rm -rf cv2_arping