################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../blockman.c \
../d_node.c \
../global.c \
../lib.c \
../sbp_perror.c 

OBJS += \
./blockman.o \
./d_node.o \
./global.o \
./lib.o \
./sbp_perror.o 

C_DEPS += \
./blockman.d \
./d_node.d \
./global.d \
./lib.d \
./sbp_perror.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


