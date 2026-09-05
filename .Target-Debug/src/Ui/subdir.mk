################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ui/ChatForm.cpp \
../src/Ui/ContactListForm.cpp \
../src/Ui/FormNavigator.cpp \
../src/Ui/LoginForm.cpp \
../src/Ui/ProfileForm.cpp \
../src/Ui/SettingsForm.cpp 

OBJS += \
./src/Ui/ChatForm.o \
./src/Ui/ContactListForm.o \
./src/Ui/FormNavigator.o \
./src/Ui/LoginForm.o \
./src/Ui/ProfileForm.o \
./src/Ui/SettingsForm.o 

CPP_DEPS += \
./src/Ui/ChatForm.d \
./src/Ui/ContactListForm.d \
./src/Ui/FormNavigator.d \
./src/Ui/LoginForm.d \
./src/Ui/ProfileForm.d \
./src/Ui/SettingsForm.d 


# Each subdirectory must supply rules for building sources it contributes
src/Ui/%.o: ../src/Ui/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: bada C++ Compiler'
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -c -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	arm-samsung-nucleuseabi-g++ -D_DEBUG -DSHP -I"C:/bada/2.0.6/Include" -I"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/inc" -O0 -g -Wall -E -fpic -fshort-wchar -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -o"C:/Users/Vitaliy/Documents/bada/IDE/workspace/repository/AGG/Target-Debug/$(notdir $(basename $@).i)" "$<"
	@echo 'Finished building: $<'
	@echo ' '


