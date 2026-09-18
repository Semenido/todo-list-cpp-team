# Makefile для проекта todo-list-cpp-team

PRO_FILE   = todo-list.pro
MAKEFILE   = Makefile.qmake
TARGET     = todo-list

.PHONY: all clean run rebuild qmake

all: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE)

$(MAKEFILE): $(PRO_FILE)
	qmake $(PRO_FILE) -o $(MAKEFILE)

run: all
	./$(TARGET)

clean:
	@if [ -f $(MAKEFILE) ]; then $(MAKE) -f $(MAKEFILE) clean; fi
	@rm -f $(MAKEFILE) $(TARGET) *.o
	@rm -rf .qmake.stash

rebuild: clean all

qmake: $(MAKEFILE)