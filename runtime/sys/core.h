#pragma once

// Called when something goes very wrong. Currently, this is used instead of exceptions.
// When `sys_panic` is called, the system cannot recover, and must be restarted in order
// to continue.
void sys_panic(const char* message);