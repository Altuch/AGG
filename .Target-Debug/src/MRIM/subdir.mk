################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/MRIM/MrimAuth.cpp \
../src/MRIM/MrimContacts.cpp \
../src/MRIM/MrimMessages.cpp \
../src/MRIM/MrimUtils.cpp 

OBJS += \
./src/MRIM/MrimAuth.o \
./src/MRIM/MrimContacts.o \
./src/MRIM/MrimMessages.o \
./src/MRIM/MrimUtils.o 

CPP_DEPS += \
./src/MRIM/MrimAuth.d \
./src/MRIM/MrimContacts.d \
./src/MRIM/MrimMessages.d \
./src/MRIM/MrimUtils.d 


# Each subdirectory must supply rules for building sources it contributes
src/MRIM/%.o: ../src/MRIM/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: bada C++ Compiler'
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -c -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -E -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -o"C:/Users/Vitaliy/Documents/bada/IDE/workspace/repository/AGG/Target-Debug/$(notdir $(basename $@).i)" "$<"
	@echo 'Finished building: $<'
	@echo ' '


