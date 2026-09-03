################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

REPOSITORY_ROOT := C:/Users/Vitaliy/Documents/bada/IDE/workspace/repository/AGG/Target-Debug

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include src/MRIM/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: AGG.exe

# Tool invocations
AGG.exe: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: bada C++ Linker'
	@echo 'ldflags: -march=armv7-a -mlittle-endian -mthumb -mthumb-interwork -mfpu=vfpv3 -mfloat-abi=hard -Xlinker -lm -Xlinker -lc -L"C:/bada/2.0.6/Model/WaveWVGA/Target" -L"C:/Users/Vitaliy/Documents/bada/IDE/workspace/AGG/lib" -shared -o"AGG.exe" ./src/AGG.o ./src/AGGEntry.o ./src/AggConnection.o ./src/CONTACT.o ./src/ChatForm.o ./src/ContactListForm.o ./src/Form1.o ./src/Settings.o ./src/MRIM/MrimAuth.o ./src/MRIM/MrimContacts.o ./src/MRIM/MrimMessages.o ./src/MRIM/MrimUtils.o '
	arm-samsung-nucleuseabi-g++ @ldflags @ldobjs  $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(CXX_DEPS)$(C_UPPER_DEPS) AGG.exe $(REPOSITORY_ROOT)/temp/*  $(REPOSITORY_ROOT)/*
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
