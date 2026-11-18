################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/uart/subdir.mk
-include src/srec/subdir.mk
-include src/platform/subdir.mk
-include src/menu/subdir.mk
-include src/flash/subdir.mk
-include src/diagnostic/subdir.mk
-include src/control/subdir.mk
-include src/bootstrap/subdir.mk
-include src/app/subdir.mk
-include src/subdir.mk
-include Project_Settings/Startup_Code/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_SIZE += \
S32K144_BOOTLOADER_MOCK_TEST.siz \


# All Target
all: S32K144_BOOTLOADER_MOCK_TEST.elf secondary-outputs

# Tool invocations
S32K144_BOOTLOADER_MOCK_TEST.elf: $(OBJS) F:/NXP_IDE/S32K_Fresher/S32K144_BOOTLOADER_MOCK_TEST/Project_Settings/Linker_Files/S32K144_64_flash.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Standard S32DS C Linker'
	arm-none-eabi-gcc -o "S32K144_BOOTLOADER_MOCK_TEST.elf" "@S32K144_BOOTLOADER_MOCK_TEST.args"  $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

S32K144_BOOTLOADER_MOCK_TEST.siz: S32K144_BOOTLOADER_MOCK_TEST.elf
	@echo 'Invoking: Standard S32DS Print Size'
	arm-none-eabi-size --format=berkeley S32K144_BOOTLOADER_MOCK_TEST.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ./*
	-@echo ' '

secondary-outputs: $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
