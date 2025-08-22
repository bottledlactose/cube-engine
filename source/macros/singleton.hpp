#pragma once

// This macro is used to create a singleton class. It should be placed in the body of the class.
#define MAKE_SINGLETON(class_name) \
    private: \
        class_name() = default; \
        class_name(const class_name&) = delete; \
        class_name& operator=(const class_name&) = delete; \
    public: \
        static class_name& Get() { \
            static class_name instance; \
            return instance; \
        }
