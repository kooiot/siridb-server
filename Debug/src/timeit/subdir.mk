# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../src/timeit/timeit.c

OBJS += \
./src/timeit/timeit.o

C_DEPS += \
./src/timeit/timeit.d


# Each subdirectory must supply rules for building sources it contributes
src/timeit/%.o: ../src/timeit/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) -I../include -O0 -g3 -Wall -Wextra $(CPPFLAGS) $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


