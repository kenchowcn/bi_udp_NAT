all:
	gcc -o bi_udp_phone bi_udp_phone.c
	gcc -o bi_udp_PC bi_udp_PC.c

clean:
	rm -rf bi_udp_phone bi_udp_PC
