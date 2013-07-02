
#define MINVALUE -32768

class RingBuffer
{
    public:
        RingBuffer(int size, const char* name);
        RingBuffer();
        void cleanup(int start, int end);
        void cleanup();
        int avg();
        int get_last_avg();
        void json(char *jsonbuf);
        bool store(int value, int slot);

    private:
        int * buffer;
        int i;
        int buf_len;
        int index;  
        char name_[4];
        char jsontmp[10];
        int last_average;
};
