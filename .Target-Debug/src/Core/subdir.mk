################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Core/AggConnection.cpp \
../src/Core/AppSettings.cpp \
../src/Core/AvatarLoader.cpp \
../src/Core/ChatHistory.cpp \
../src/Core/ContactSync.cpp \
../src/Core/MessageRouter.cpp 

OBJS += \
./src/Core/AggConnection.o \
./src/Core/AppSettings.o \
./src/Core/AvatarLoader.o \
./src/Core/ChatHistory.o \
./src/Core/ContactSync.o \
./src/Core/MessageRouter.o 

CPP_DEPS += \
./src/Core/AggConnection.d \
./src/Core/AppSettings.d \
./src/Core/AvatarLoader.d \
./src/Core/ChatHistory.d \
./src/Core/ContactSync.d \
./src/Core/MessageRouter.d 


# Each subdirectory must supply rules for building sources it contributes
src/Core/%.o: ../src/Core/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: bada C++ Compiler'
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -c -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -E -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -o"C:/Users/Vitaliy/Documents/bada/IDE/workspace/repository/AGG/Target-Debug/$(notdir $(basename $@).i)" "$<"
	@echo 'Finished building: $<'
	@echo ' '


