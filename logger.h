#ifndef LOGGER_H
#define LOGGER_H

void log_setfile(const char *filename);
void log_write(const char *format, ...);

#endif
