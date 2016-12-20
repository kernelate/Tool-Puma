SOURCE_FILE_SERVER= \
    multiple_server.c \
	socket_server_api.c
SOURCE_FILE_CLIENT= \
    test_client.c	\
	reply.c			\
	check.c

ARM_COMPILER='/home/ntekcom11/FriendlyARM/toolschain/4.5.1/bin/arm-linux-gcc'

FILE_SERVER=test_server_diana
FILE_CLIENT=test_client_diana
NATIVE_FILENAME_SERVER=$(FILE_SERVER)_native
NATIVE_FILENAME_CLIENT=$(FILE_CLIENT)_native
NATIVE_PARM=-lpthread
ARM_FILENAME_SERVER=$(FILE_SERVER)_arm
ARM_FILENAME_CLIENT=$(FILE_CLIENT)_arm
ARM_PARM=-static -lpthread

default:
	clear && gcc $(SOURCE_FILE_SERVER) $(NATIVE_PARM) -o $(NATIVE_FILENAME_SERVER) # && gcc $(SOURCE_FILE_CLIENT) $(NATIVE_PARM) -o $(NATIVE_FILENAME_CLIENT)
clean:
	rm $(NATIVE_FILENAME_SERVER) $(NATIVE_FILENAME_CLIENT)
arm:
	$(ARM_COMPILER) $(SOURCE_FILE_CLIENT) $(ARM_PARM) -o $(NATIVE_FILENAME_CLIENT)
native:
	$(SOURCE_FILE) $(PARM)
