#include <thread.h>
#include <thread-sync.h>

mutex_t lk = MUTEX_INIT();

int n, depth = 0;

void T_produce() {
    while (1) {
retry:
        mutex_lock(&lk);
        if (!(depth < n)) {
            mutex_unlock(&lk);
            goto retry;
        }

        // 这样就对了，因为如果你满足条件了，就自始至终不放开这把锁

        // The check of sync condition (depth < n) is within
        // the same critical section. As long as we safely
        // protected the shared state, this condition should
        // always hold at this point.
        assert(depth < n);

        printf("(");
        depth++;

        // And at this point, the condition (depth > 0) is
        // satisfied. However, a consumer could proceed with
        // checking depth only if the lock is released.
        mutex_unlock(&lk);
    }
}

void T_consume() {
    while (1) {
retry:
        mutex_lock(&lk);
        if (!(depth > 0)) {
            mutex_unlock(&lk);
            goto retry;
        }

        assert(depth > 0);

        printf(")");
        depth--;

        mutex_unlock(&lk);
    }
}
