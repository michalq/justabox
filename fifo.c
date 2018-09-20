
typedef struct fifo_t {
    uint8_t* buf;
    uint8_t head;
    uint8_t tail;
    uint8_t size;
} fifo_t;
namespace {
    void fifo_init(fifo_t *f, uint8_t *buf, uint8_t size) {
        f->head = 0;
        f->tail = 0;
        f->buf = buf;
        f->size = size;
    }

    uint8_t fifo_push(fifo_t *f, uint8_t val) {
        if (f->head + 1 == f->tail || (f->head + 1 == f->size && 0 == f->tail)) {
            return 0;
        }

        f->buf[f->head] = val;
        f->head++;
        if (f->head == f->size) {
            f->head = 0;
        }

        return 1;
    }

    uint8_t fifo_pop(fifo_t *f, uint8_t *val) {
        if (f->tail == f->head) {
            return 0;
        }

        uint8_t tmp = f->buf[f->tail];
        f->tail++;
        if (f->tail == f->size) {
            f->tail = 0;
        }

        *val = tmp;

        return 1;
    }
}
