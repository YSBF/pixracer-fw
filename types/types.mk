TYPEC = python3 lib/mcucom/ts/type_compiler/type_compiler.py
TYPE_DIR := types

TYPE_OUT_DIR = $(BUILDDIR)/types

TYPEFILES = $(wildcard $(TYPE_DIR)/*.type)
TYPECSRC = $(addprefix $(TYPE_OUT_DIR)/,$(notdir $(TYPEFILES:.type=.c)))
TYPEINC = $(BUILDDIR)

%.c %.h : %.type $(GLOBAL_SRC_DEP)
	@mkdir -p $(TYPE_OUT_DIR)
	@echo "Compiling type headers for $<"
	$(TYPEC) $< -o $(TYPE_OUT_DIR)/$@
