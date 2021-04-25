#pragma once

void enableDebugging();
void initializeDebugger();
void glDebugOutput(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char *message, const void *userParam);