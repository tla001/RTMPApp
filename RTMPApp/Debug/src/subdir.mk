################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/RTMPPushFlv.cpp \
../src/RTMPPushH264.cpp \
../src/RTMPRec.cpp \
../src/RtmpH264.cpp \
../src/ThreadBase.cpp \
../src/librtmp_send264.cpp \
../src/sockInit.cpp 

OBJS += \
./src/RTMPPushFlv.o \
./src/RTMPPushH264.o \
./src/RTMPRec.o \
./src/RtmpH264.o \
./src/ThreadBase.o \
./src/librtmp_send264.o \
./src/sockInit.o 

CPP_DEPS += \
./src/RTMPPushFlv.d \
./src/RTMPPushH264.d \
./src/RTMPRec.d \
./src/RtmpH264.d \
./src/ThreadBase.d \
./src/librtmp_send264.d \
./src/sockInit.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/tla001/workspace/RTMPApp/inc" -I"/home/tla001/workspace/RTMPApp/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


