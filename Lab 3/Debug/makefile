################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./main.obj" \
"./peripherals.obj" \
"./LcdDriver/HAL_MSP_EXP430FR5529_Sharp96x96.obj" \
"./LcdDriver/Sharp96x96.obj" \
"./fonts/fontfixed6x8.obj" \
"./images/LPRocket_96x37.obj" \
"./images/TI_Logo_69x64.obj" \
"../lnk_msp430f5529.cmd" \
"../lib/grlib.lib" \
$(GEN_CMDS__FLAG) \
-llibmath.a \
-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include LcdDriver/subdir_vars.mk
-include fonts/subdir_vars.mk
-include images/subdir_vars.mk
-include lib/subdir_vars.mk
-include subdir_rules.mk
-include LcdDriver/subdir_rules.mk
-include fonts/subdir_rules.mk
-include images/subdir_rules.mk
-include lib/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
Lab2.out 

EXE_OUTPUTS__QUOTED += \
"Lab2.out" 


# All Target
all: $(OBJS) $(CMD_SRCS) $(LIB_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "Lab2.out"

# Tool invocations
Lab2.out: $(OBJS) $(CMD_SRCS) $(LIB_SRCS) $(GEN_CMDS)
	@echo 'MSP430 Linker - building target: "$@"'
	"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --advice:power=all --define=__MSP430F5529__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 -z -m"Lab2.map" --heap_size=160 --stack_size=160 --cinit_hold_wdt=on -i"C:/ti/ccs2050/ccs/ccs_base/msp430/include" -i"C:/Users/Binary/Desktop/ECE 2049/Lab2/Lab2" -i"C:/ti/ccs2050/ccs/ccs_base/msp430/lib/5xx_6xx_FRxx" -i"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/lib" -i"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="Lab2_linkInfo.xml" --use_hw_mpy=F5 --rom_model -o "Lab2.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "main.obj" "peripherals.obj" "LcdDriver\HAL_MSP_EXP430FR5529_Sharp96x96.obj" "LcdDriver\Sharp96x96.obj" "fonts\fontfixed6x8.obj" "images\LPRocket_96x37.obj" "images\TI_Logo_69x64.obj" 
	-$(RM) "main.d" "peripherals.d" "LcdDriver\HAL_MSP_EXP430FR5529_Sharp96x96.d" "LcdDriver\Sharp96x96.d" "fonts\fontfixed6x8.d" "images\LPRocket_96x37.d" "images\TI_Logo_69x64.d" 
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

