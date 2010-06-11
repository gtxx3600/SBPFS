################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../chmod.c \
../chown.c \
../global.c \
../lib.c \
../login.c \
../move.c \
../open.c \
../read_write.c \
../remove.c \
../sbp_perror.c 

OBJS += \
./chmod.o \
./chown.o \
./global.o \
./lib.o \
./login.o \
./move.o \
./open.o \
./read_write.o \
./remove.o \
./sbp_perror.o 

C_DEPS += \
./chmod.d \
./chown.d \
./global.d \
./lib.d \
./login.d \
./move.d \
./open.d \
./read_write.d \
./remove.d \
./sbp_perror.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


