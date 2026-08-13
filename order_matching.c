#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define HASH_SIZE 4096
#define SIZE 100000

typedef struct Order {
    int id;
    char side;
    double price;
    int qty;
    int is_market;
    struct Order *next;
    struct Order *prev;
    struct PriceLvl *lvl;
} Order;

typedef struct PriceLvl {
    double price;
    Order *first_order;
    Order *last_order;
    struct PriceLvl *next;
    struct PriceLvl *prev;
} PriceLvl;

typedef struct Trade {
    int buy_id;
    int sell_id;
    double price;
    int qty;
} Trade;

typedef struct OrderSpec {
    int id;
    char side;
    double price;
    int qty;
    int is_market;
} OrderSpec;

typedef struct HashEntry {
    int id;
    Order *order;
    struct HashEntry *next;
} HashEntry;

HashEntry *order_index[HASH_SIZE];

int hash_id(int id)
{
    return ((unsigned int)id) % HASH_SIZE;
}

void hash_insert(int id, Order *order)
{
    int idx = hash_id(id);
    HashEntry *node = malloc(sizeof(HashEntry));
    node->id = id;
    node->order = order;
    node->next = order_index[idx];
    order_index[idx] = node;
}

Order *hash_lookup(int id)
{
    int idx = hash_id(id);
    HashEntry *node = order_index[idx];
    while (node != NULL)
    {
        if (node->id == id) return node->order;
        node = node->next;
    }
    return NULL;
}

void hash_remove(int id)
{
    int idx = hash_id(id);
    HashEntry *node = order_index[idx];
    HashEntry *prev = NULL;
    while (node != NULL)
    {
        if (node->id == id)
        {
            if (prev == NULL) order_index[idx] = node->next;
            else prev->next = node->next;
            free(node);
            return;
        }
        prev = node;
        node = node->next;
    }
}

Order *create_order(int id, char side, double price, int qty, int is_market) {
    Order *new_order = malloc(sizeof(Order));
    new_order->id = id;
    new_order->side = side;
    new_order->price = price;
    new_order->qty = qty;
    new_order->is_market = is_market;
    new_order->next = NULL;
    new_order->prev = NULL;
    new_order->lvl = NULL;
    return new_order;
}

void add_order_to_lvl(PriceLvl *lvl, Order *new_order)
{
    new_order->lvl = lvl;
    if (lvl->first_order == NULL)
    {
        lvl->first_order = new_order;
        lvl->last_order = new_order;
        new_order->prev = NULL;
    }
    else
    {
        lvl->last_order->next = new_order;
        new_order->prev = lvl->last_order;
        lvl->last_order = new_order;
    }
    hash_insert(new_order->id, new_order);
}

PriceLvl *find_or_create_lvl(PriceLvl **first_ptr, double price, char side)
{
    PriceLvl *curr = *first_ptr;
    PriceLvl *prev = NULL;

    while (curr != NULL)
    {
        if (curr->price == price)
        {
            return curr;
        }
        if (side == 'B' && curr->price < price) break;
        if (side == 'A' && curr->price > price) break;
        prev = curr;
        curr = curr->next;
    }

    PriceLvl *new_level = malloc(sizeof(PriceLvl));
    new_level->price = price;
    new_level->first_order = NULL;
    new_level->last_order = NULL;
    new_level->next = curr;
    new_level->prev = prev;

    if (prev == NULL)
    {
        *first_ptr = new_level;
    }
    else
    {
        prev->next = new_level;
    }

    if (curr != NULL)
    {
        curr->prev = new_level;
    }

    return new_level;
}

int match_order(Order *inc, PriceLvl **first_opp, char side, Trade trades[], int *trade_ct)
{
    PriceLvl *best = *first_opp;

    while (best != NULL && inc->qty > 0)
    {
        if (!inc->is_market)
        {
            if (side == 'B' && inc->price < best->price) break;
            if (side == 'A' && inc->price > best->price) break;
        }

        Order *rem = best->first_order;
        while (rem != NULL && inc->qty > 0)
        {
            int fill_qty = (inc->qty < rem->qty) ? inc->qty : rem->qty;

            trades[*trade_ct].buy_id  = (side == 'B') ? inc->id : rem->id;
            trades[*trade_ct].sell_id = (side == 'B') ? rem->id : inc->id;
            trades[*trade_ct].price   = best->price;
            trades[*trade_ct].qty     = fill_qty;
            (*trade_ct)++;

            inc->qty -= fill_qty;
            rem->qty  -= fill_qty;

            if (rem->qty == 0)
            {
                Order *to_remove = rem;
                Order *next_order = rem->next;

                if (to_remove->prev == NULL) best->first_order = next_order;
                else to_remove->prev->next = next_order;

                if (next_order == NULL) best->last_order = to_remove->prev;
                else next_order->prev = to_remove->prev;

                hash_remove(to_remove->id);
                free(to_remove);
                rem = next_order;
            }
            else
            {
                rem = rem->next;
            }
        }

        if (best->first_order == NULL)
        {
            PriceLvl *empty_lvl = best;
            PriceLvl *before = empty_lvl->prev;
            PriceLvl *after = empty_lvl->next;

            if (before == NULL) *first_opp = after;
            else before->next = after;

            if (after != NULL) after->prev = before;

            best = after;
            free(empty_lvl);
        }
        else
        {
            best = best->next;
        }
    }

    return inc->qty;
}

void add_order(Order *new_order, PriceLvl **bids, PriceLvl **asks, Trade trades[], int *trade_ct)
{
    if (new_order->side == 'B')
    {
        int leftover = match_order(new_order, asks, 'B', trades, trade_ct);
        if (leftover > 0)
        {
            if (new_order->is_market)
            {
                free(new_order);
            }
            else
            {
                new_order->qty = leftover;
                add_order_to_lvl(find_or_create_lvl(bids, new_order->price, 'B'), new_order);
            }
        }
    }
    else
    {
        int leftover = match_order(new_order, bids, 'A', trades, trade_ct);
        if (leftover > 0)
        {
            if (new_order->is_market)
            {
                free(new_order);
            }
            else
            {
                new_order->qty = leftover;
                add_order_to_lvl(find_or_create_lvl(asks, new_order->price, 'A'), new_order);
            }
        }
    }
}

int cancel_order(int id, PriceLvl **bids, PriceLvl **asks)
{
    Order *target = hash_lookup(id);
    if (target == NULL) return 0;

    char side = target->side;
    PriceLvl *lvl = target->lvl;

    if (target->prev == NULL) lvl->first_order = target->next;
    else target->prev->next = target->next;

    if (target->next == NULL) lvl->last_order = target->prev;
    else target->next->prev = target->prev;

    hash_remove(id);
    free(target);

    if (lvl->first_order == NULL)
    {
        PriceLvl *before = lvl->prev;
        PriceLvl *after = lvl->next;

        if (before == NULL)
        {
            if (side == 'B') *bids = after;
            else *asks = after;
        }
        else
        {
            before->next = after;
        }

        if (after != NULL) after->prev = before;

        free(lvl);
    }

    return 1;
}

void print_results(Trade trades[], int trade_ct)
{
    if (trade_ct == 0)
    {
        printf("No trades executed.\n");
        return;
    }

    int total_qty = 0;
    double total_val = 0.0;

    for (int i = 0; i < trade_ct; i++)
    {
        total_qty += trades[i].qty;
        total_val += trades[i].price * trades[i].qty;
    }

    double vwap = total_val / total_qty;

    printf("- Trade Summary -\n");
    printf("Total trades: %d\n", trade_ct);
    printf("Total volume: %d\n", total_qty);
    printf("VWAP: %.2f\n", vwap);
}

OrderSpec random_order(int id)
{
    OrderSpec spec;
    spec.id = id;
    spec.side = (rand() % 2 == 0) ? 'B' : 'A';
    spec.is_market = (rand() % 5 == 0) ? 1 : 0;
    spec.price = spec.is_market ? 0.0 : 99.50 + (rand() % 100) / 100.0;
    spec.qty = (rand() % 10 == 0) ? (50 + rand() % 200) : (1 + rand() % 20);
    return spec;
}

void print_summary(PriceLvl *book, const char *label, int top_n)
{
    printf("- %s Summary -\n", label);
    PriceLvl *lvl = book;
    int shown = 0;
    int total_levels = 0;
    int total_orders = 0;
    int total_qty = 0;

    while (lvl != NULL)
    {
        int level_qty = 0;
        int level_orders = 0;
        Order *ord = lvl->first_order;
        while (ord != NULL)
        {
            level_qty += ord->qty;
            level_orders++;
            ord = ord->next;
        }

        if (shown < top_n)
        {
            printf("  Price %.2f: %d orders, qty=%d\n", lvl->price, level_orders, level_qty);
            shown++;
        }

        total_levels++;
        total_orders += level_orders;
        total_qty += level_qty;
        lvl = lvl->next;
    }

    printf("  (%d total price levels, %d total orders, %d total qty)\n", total_levels, total_orders, total_qty);
}

int main(void)
{
    srand(time(NULL));

    PriceLvl *bids = NULL;
    PriceLvl *asks = NULL;
    Trade *trades = malloc(SIZE * sizeof(Trade));
    int trade_ct = 0;
    int cancels_successful = 0;
    int market_order_ct = 0;

    int num_orders = SIZE;

    clock_t start = clock();

    for (int i = 1; i <= num_orders; i++)
    {
        OrderSpec spec = random_order(i);
        if (spec.is_market) market_order_ct++;
        Order *o = create_order(spec.id, spec.side, spec.price, spec.qty, spec.is_market);
        add_order(o, &bids, &asks, trades, &trade_ct);

        if (rand() % 100 == 0 && bids != NULL)
        {
            int canceled_id = bids->first_order->id;
            int result = cancel_order(canceled_id, &bids, &asks);
            if (result) cancels_successful++;
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Processed %d orders in %f seconds\n\n", num_orders, elapsed);
    printf("Market orders: %d | Limit orders: %d\n\n", market_order_ct, num_orders - market_order_ct);

    if (bids != NULL && asks != NULL)
    {
        printf("Best Bid: %.2f | Best Ask: %.2f | Spread: %.2f\n\n",
               bids->price, asks->price, asks->price - bids->price);
    }

    print_summary(bids, "Bids", 5);
    printf("\n");
    print_summary(asks, "Asks", 5);
    printf("\n");
    print_results(trades, trade_ct);
    printf("Orders canceled: %d\n", cancels_successful);

    free(trades);

    return 0;
}