*** INSTRUCTIONS FOR COMPILING AND DOWNLOADING SCIF TUTORIALS ***

1. SCIF tutorials directory structure
tutorials:  1) .c source files
	    2) Makefile, README.txt, launch.sh
	    3) windows - scif_tutorials.sln, VS proj files
	    4) windows\output - windows built executables

2. Accessing tutorials from rpm or msi

Linux: You will also have to install additional dependency rpms.
Host Driver:
1) mpss-bootimage or mpss-boot-files
2) mpss-daemon
3) mpss-modules
Scif Shared Object Library:
4) libscif0
Scif Dev Packages:
5) mpss-modules-headers
6) mpss-modules-headers-dev or mpss-modules-dev
7) libscif-dev
Install scif tutorials from:
mpss-sciftutorials-doc
The source files are installed into /usr/share/doc/scif/tutorials.

You will also have to install the MPSS SDK into /opt/mpss

Windows: Scif tutorials are installed as part of msi installation package
in C:\Program Files\Intel\MPSS\sdk\tutorials\scif.

3. Start MPSS Software Stack:
	linux: sudo service mpss start
	windows: micctrl --start
You can verify the card status using:
	linux: sudo micctrl -s
	windows: micctrl -s
It should be online.

4. Compile SCIF tutorials:

Linux : From the Tutorials directory, compile all SCIF tutorials.
for host binaries, do:
make target_arch=x86_64
for card binaries, do:
(. /opt/mpss/<version>/environment-setup-k1om-mpss-linux && make target_arch=k1om)
where <version> is the MPSS version.

Windows : Should have Visual Studio 2012 installed. From tutorials\windows directory,
you can build the tutorials using scif_tutorials.sln file. The host and card 
executables are generated in windows/output directory.

5. Accessing the Embedded Intel® Xeon Phi (TM) Linux OS:

An embedded Linux OS shell prompt is available through a virtual network
connection across the PCI bus. You can access the card using ssh.
	ssh mic0 (accesses the first card)
	
On windows, you can access card using putty. You can refer to
Windows_MPSS_Boot_Config_Guide for ssh config steps on windows.

6. Download all Intel® Xeon Phi (TM) executables to the Intel® Xeon Phi (TM) card

All executables ending with _mic should be copied via the scp command to the
Intel® Xeon Phi (TM) card /tmp/ dir. As an example use "scp scif_connect_mic mic0:/tmp/"
to transfer file to /tmp/ directory on first card. All the SCIF tutorial executables
contain pairs of tests.  Each test has one copy compiled to run on the host with names
ending in “_host” and one copy to run on the embedded Intel® Xeon Phi (TM) Linux OS with
names ending in “_mic”. In a multi-card system, you can access a specific card using micN
where N is the card number.

7. Running the SCIF Sample Tests:

NOTE: When you are referring to remote_node or peer_node, host is always 0 and
Intel® Xeon Phi (TM) cards are numbered from 1 to N, where N is the total number
of available cards.

Linux : To run the tutorial, you need to execute the _host, _mic pair
on host and card respectively. Check for executable permissions of the
binaries and do chmod 755 scif_* if necesaary.

Windows : To run the tutorial, you need to execute the _host, _mic pair
on host and card respectively.

NOTE: To run the tutorials, recommended port numbers are 2048 and higher.

NOTE: Windows 7 has a limitation on mapping size. So, to run the tutorials
using scif_mmap on windows, recommended data size is 1 page. Higher values
will result in scif_mmap failure.

NOTE: All tutorials are not ported to windows due to mmap and usecase limitations.

------------------------------------------------------------------------------
Tutorial 1 : Demonstrates basic usage of scif connection APIs
	     Demonstrates:- Binding multiple end pts to a port is not allowed
	     Demonstrates:- Binding an end pt to multiple ports is not allowed
Programs   : scif_connect.c
	     scif_accept.c
------------------------------------------------------------------------------
The executables take the following command line arguments.

On host:
./scif_connect_host -l <local_port> -n <peer_node[host/card 0/1]>
	-r <remote_port> -s <msg_size> -b <block/nonblock 1/0>

On card, from telnet session:
./scif_accept_mic -l <local_port> -s <msg_size> -b <block/non-block 1/0>

The connecting & accepting sides can be changed using ./scif_connect_mic &
./scif_accept_host on card and host respectively

NOTE: 1) The msg_size specified on both sides should be same
      2) <remote_port> mentioned on connecting side & <local_port> mentioned
      on accepting side should be same value

------------------------------------------------------------------------------
Tutorial 2 : Demonstrates the use of poll() on scif_accept and send, receive
	     queues for large data transfers
Programs   : scif_connect_poll.c
	     scif_accept_poll.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_connect_poll_host -l <local_port> -n <peer_node[host/card 0/1]>
	-r <remote_port> -s <no 4k pages> -b <block/non-block 1/0>

On card, from telnet session:
./scif_accept_poll_mic -l <local_port> -s <no 4k pages> -b <block/nonblock 1/0>

The connecting & accepting sides can be changed using ./scif_connect_poll_mic
& ./scif_accept_poll_host on card and host respectively

NOTE: 1) The <no 4k pages> specified on both sides should be same
      2) <remote_port> argument on connecting side & <local_port> argument
      on accepting side should be same value

------------------------------------------------------------------------------
Tutorial 3 : Demonstrates how the remote process on Intel® Xeon Phi (TM) is
	     launched by host program (through ssh). Host arguments are passed
	     as corresponding card side arguments. The card side peer_port argument
	     is generated by host side application which then intiates a connection
	     request back to host. Also, demonstrates that closing a connected
	     end pt closes the the corresponding peer end pt.
Programs   : scif_connect_launch.c
	     scif_accept_launch.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_accept_launch_host -n <peer_node> -s <msg_size> -b <block/nonblock 1/0>
	-t <terminate_check>

Card side arguments are fed in scif_accept_launch.c and launched.
./scif_connect_launch_mic -r <peer_port> -s <msg_size>
	-b <block/nonblock 1/0> -t <terminate_check>

NOTE: 1) <terminate_check> argument is set to 1 if you want to see the
      demonstration of closing a connected end point (closes the connected
      peer end pt). Else set it to 0.
      2) launch.sh script file (in tutorials/ directory) also needs
      to be transfered to /tmp/ dir in card through scp and check for
      executable permissions.
      3) <msg_size> specified on both sides should be same.
	  4) This tutorial has been programmed to launch a remote process on
	  card from host. The reverse way doesn't work.

------------------------------------------------------------------------------
Tutorial 4 : The connecting side sends multiple connection requests to remote
	     node which handles the requests synchronously. It also
	     demonstrates that closing a listening end pt rejects further
	     connection requests
Programs   : scif_connect_multiple.c
	     scif_accept_multiple.c
------------------------------------------------------------------------------
The executables take the following command line arguments.

On host:
./scif_connect_multiple_host -n <peer_node> -l <starting_port_no> -c <no_connections>
	-s <msg_size>

On card, from telnet session:
./scif_accept_multiple_mic -s <msg_size> -e <close epd 1/0>

The connecting & accepting sides can be changed using ./scif_connect_multiple_mic
& ./scif_accept_multiple_host on card and host respectively

NOTE: 1) <msg_size> argument should have same value on both sides
      2) Setting <close_epd> argument demonstrates the effect of closing a
      listening end pt.

------------------------------------------------------------------------------
Tutorial 5 : Demonstrates basic implementation of registering windows and
	     scif_mmap calls. Also demonstrates registered window deletion.
Programs   : scif_connect_register.c
	     scif_accept_register.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_connect_register_host -n <no 4K pages> -m <map_manager 0/1/2>
	-r <remote_node> -u <unreg_check>

On card, from telnet session:
./scif_accept_register_mic -n <no 4K pages> -m <map_manager 0/1/2>
	-u <unreg_check>

The connecting & accepting sides can be changed using ./scif_connect_register_mic
& ./scif_accept_register_host on card and host respectively

NOTE: 1) <no 4k pages> argument should have same value on both sides
      2) Setting <map_manager> argument allows you to demonstrate different
      mapping managements & it should be same on both sides.
      3) Setting <unreg_check> to 1 demonstrates registered window deletion
      4) <remote_node> = 0 for host, and for card, it depends on which card
      you are using. Eg: for 1st card, <remote_node> = 1
      5) map_manager=0 : SCIF_MAP_FIXED not set, scif manages RAS; implementaion
      defined suitable offsets are chosen for mapping len bytes.

      map_manager=1 : SCIF_MAP_FIXED set, user managed; specified fixed offset
      are used. For scif_register doesnt replace existing registrations &
      returns error. However, scif_mmap replaces existing mappings.

      map_manager=2 : SCIF_MAP_FIXED set, OS managed; offset is same as virtual
      addr. This relieves the app from the need to track association between VA
      and RA as they are same.

------------------------------------------------------------------------------
Tutorial 6 : Demonstrates scif_readfrom & scif_writeto APIs and emphasizes the
	     need for having registered windows on both connected nodes.
Programs   : scif_connect_readwrite_p1.c
	     scif_connect_readwrite_p2.c
	     scif_accept_readwrite.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_connect_readwrite_p1_host -n <no 4K pages> -m <map_fixed 1/0>
	-c <cpu/dma 1/0> -r <remote_node>

./scif_connect_readwrite_p2_host -n <no 4K pages> -m <map_fixed 1/0>
        -c <cpu/dma 1/0> -r <remote_node>

On card, from telent session:
./scif_accept_readwrite_mic -n <no 4K pages> -m <map_fixed 0/1>

The connecting & accepting sides can be changed using ./scif_accept_readwrite_host
on host & ./scif_connect_readwrite_p1_mic, ./scif_connect_readwrite_p2_mic on card

NOTE: 1) <no 4k pages> should have same value on both sides
      2) <map_fixed> value should be same on both sides
      3) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.
      4) Have to execute both *_p1_*, *_p2_* on connecting side to run
      the tutorial.

------------------------------------------------------------------------------
Tutorial 7 : Demonstrates scif_vwriteto & scif_vreadfrom APIs and emphasizes
	     the need for having registered window only on the node to which
	     data is being written to or read from. Demonstrates circumstances
	     when it may be advantageous to use scif_vwriteto() and
	     scif_vreadfrom() rather than scif_writeto() and scif_readfrom().
Programs   : scif_connect_vreadwrite_p1.c
	     scif_connect_vreadwrite_p2.c
	     scif_accept_vreadwrite.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_connect_vreadwrite_p1_host -n <no 4K pages> -c <cpu/dma 1/0>
	-r <remote_node>

./scif_connect_vreadwrite_p2_host -n <no 4K pages> -c <cpu/dma 1/0>
	-r <remote_node>

On card, from telnet session:
./scif_accept_vreadwrite_mic -n <no 4K pages> -m <map_fixed 0/1>

The connecting & accepting sides can be changed using
./scif_accept_vreadwrite_host on host & ./scif_connect_vreadwrite_p1_mic,
./scif_connect_vreadwrite_p2_mic on card

NOTE: 1) <no 4k pages> should have same value on both sides
      2) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.
      3) Have to execute both *_p1_*, *_p2_* on connecting side to run
      the tutorial.

------------------------------------------------------------------------------
Tutorial 8 : Demonstrates mapping of multiple windows over same physical address
	     space. Processes P1, P2 read/write to same phy addr pages of peer
	     through peer's registered windows.
programs   : scif_connect_rma_register_p1.c
	     scif_connect_rma_register_p2.c
	     scif_accept_rma_register.c
------------------------------------------------------------------------------
The executables take the following command line arguments.

On host:
./scif_connect_rma_register_p1_host -n <no 4K pages> -c <cpu/dma 1/0>
	-r <remote_node>

./scif_connect_rma_register_p2_host -n <no 4K pages> -c <cpu/dma 1/0>
	-r <remote_node>

On card, from telnet session:
./scif_accept_rma_register_mic -n <no 4K pages> -m <map_fixed 0/1>

The connecting & accepting sides can be changed using
./scif_accept_rma_register_host on host & ./scif_connect_rma_register_p1_mic,
./scif_connect_rma_register_p2_mic on card

NOTE: 1) <no 4k pages> argument should have same value on both sides
      2) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.
      3) Need to execute scif_connect_rma_register_p1_* before executing
      scif_connect_rma_register_p2_* on connecting side.

------------------------------------------------------------------------------
Tutorial 9 : Demonstrates scif_mmap over multiple contiguous windows
Programs   : scif_connect_rma_mmap.c
	     scif_accept_rma_mmap.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_connect_rma_mmap_host -n <no 4K pages> -r <remote_node>

On card:
./scif_accept_rma_mmap_mic -n <no 4K pages>

The connecting & accepting sides can be changed using ./scif_accept_rma_mmap_host
and ./scif_connect_rma_mmap_mic on host & card respectively.

NOTE: 1) <no 4k pages> argument on accepting side must be less than or equal to
      the value on connecting side
      2) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.

------------------------------------------------------------------------------
Tutorial 10 : Demonstrates RMA across fork() with MADV_DONTFORK flag
Programs    : scif_connect_rma_fork.c
	      scif_accept_rma_fork.c
------------------------------------------------------------------------------
The executables take following command line arguments.

On host:
./scif_connect_rma_fork_host -n <no 4K pages> -f <fork 1/0> -m <map_fixed>
	-c <cpu/dma 0/1> -r <remote_node>

On card, from telnet session:
./scif_accept_rma_fork_mic -n <no 4K pages> -f <fork 1/0> -m <map_fixed>
	-c <cpu/dma 1/0>

The connecting & accepting sides can be changed using ./scif_accept_rma_fork_host
and ./scif_connect_rma_fork_mic on host & card respectively.

NOTE: 1) <no 4k pages> argument should have same value on both sides
      2) <map_fixed> value should be same on both sides
      3) Setting <fork> argument to 1, marks Virtual Address Space as DONTFORK
      4) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.

------------------------------------------------------------------------------
Tutorial 11 : This program demonstrates uni-directional user space messaging
	      mechanism through ring buffer implementation.
Programs    : scif_connect_ringbuffer.c
	      scif_accept_ringbuffer.c
------------------------------------------------------------------------------
The executables take the following command line arguments.

On host:
./scif_connect_ringbuffer_host -n <no 4K pages> -m <map_fixed 0/1>
	-r <remote_node>

On card, from telent session:
./scif_accept_ringbuffer_mic -n <no 4K pages>

The connecting & accepting sides can be changed using
./scif_accept_ringbuffer_host and ./scif_connect_ringbuffer_mic on host & card
respectively.

NOTE: 1) <no 4k pages> argument should have same value on both sides
      2) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.

------------------------------------------------------------------------------
Tutorial 12 : This program demonstrates the need for synchronization and how
	      it is achieved through scif_fence_signal(), scif_fence_mark()
	      and scif_fence_wait()
Programs    : scif_connect_rma_fence.c
	      scif_accept_rma_fence.c
------------------------------------------------------------------------------
The executables take the following command line arguments.

On host:
./scif_connect_rma_fence_host -n <no 4K pages> -m <map_fixed 1/0>
	-c <cpu/dma 1/0> -r <remote_node>

On card, from telnet session:
./scif_accept_rma_fence_mic -n <no 4K pages> -m <map_fixed 0/1>

The connecting & accepting sides can be changed using
./scif_accept_rma_fence_host and ./scif_connect_rma_fence_mic on host & card
respectively.

NOTE: 1) <no 4k pages> argument should have same value on both sides
      2) <map_fixed> value should be same on both sides
      3) <remote_node> for host = 0 and for Intel® Xeon Phi (TM) cards,
      it starts from 1.

