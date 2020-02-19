echo "Start compiling echoServer for local environment..."
echo "local environment skipped....."
#gcc -g echoServer.c -o echoServer -levent
echo "End of the compiliation for local environment..."
echo "Start compiling eventserv for 7688 environment(Don't forget to source related file)..."
echo "7688 environment skipped....."
#$CGCC -g echoServer.c -o echoServer_7688 -levent
#$CGCC eventserv.c -shared -I/home/steven/7688_event/libevent_so/ -o eventserv_7688
echo "End of the compiliation for 7688 environment..."
echo "Start compiling eventserv for ARM(MYiR) environment(Don't forget to source related file)..."
arm-poky-linux-gnueabi-gcc -march=armv7ve -marm -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 --sysroot=/opt/myir-imx6fb/4.1.15-2.0.1/sysroots/cortexa7hf-neon-poky-linux-gnueabi -g -fPIC echoServer.c -o echoServer_arm -levent
echo "End of the compiliation for ARM(MYiR) environment..."
