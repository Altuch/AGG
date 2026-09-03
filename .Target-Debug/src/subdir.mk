################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/AGG.cpp \
../src/AGGEntry.cpp \
../src/AggConnection.cpp \
../src/CONTACT.cpp \
../src/ChatForm.cpp \
../src/ContactListForm.cpp \
../src/Form1.cpp \
../src/Settings.cpp 

OBJS += \
./src/AGG.o \
./src/AGGEntry.o \
./src/AggConnection.o \
./src/CONTACT.o \
./src/ChatForm.o \
./src/ContactListForm.o \
./src/Form1.o \
./src/Settings.o 

CPP_DEPS += \
./src/AGG.d \
./src/AGGEntry.d \
./src/AggConnection.d \
./src/CONTACT.d \
./src/ChatForm.d \
./src/ContactListForm.d \
./src/Form1.d \
./src/Settings.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: bada C++ Compiler'
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -c -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -E -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -o"C:/Users/Vitaliy/Documents/bada/IDE/workspace/repository/AGG/Target-Debug/$(notdir $(basename $@).i)" "$<"
	@echo 'Finished building: $<'
	@echo ' '


