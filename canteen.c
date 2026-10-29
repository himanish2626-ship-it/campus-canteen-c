#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_FILE     "menu.txt"
#define SALES_FILE    "sales.txt"
#define ORDER_FILE    "orderno.txt"
#define FIRST_ORDER   1000    
#define MAX_CART      50
#define GST_RATE      0.05f

typedef struct {
    int   id;
    char  name[32];
    float price;
    int   stock;
} Item;

typedef struct {
    Item *items;
    int   count;
    int   capacity;
} Menu;

typedef struct {
    int idx; 
    int qty;
} CartLine;

typedef struct {
    CartLine lines[MAX_CART];
    int      count;
} Cart;

static Menu menu; 
void cleanup(void) {
    free(menu.items);
    menu.items = NULL;
}


void readLine(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin) == NULL) {   
        printf("\n");
        exit(0);
    }
    buf[strcspn(buf, "\n")] = '\0';
}

int readInt(const char *prompt) {
    char buf[32];
    readLine(prompt, buf, sizeof buf);
    return atoi(buf);
}

void initMenu(Menu *m) {
    m->capacity = 4;
    m->count = 0;
    m->items = (Item *)malloc(m->capacity * sizeof(Item));
    if (!m->items) { printf("Memory error!\n"); exit(1); }
}

void addToMenu(Menu *m, Item it) {
    if (m->count == m->capacity) {
        m->capacity *= 2;
        Item *tmp = (Item *)realloc(m->items, m->capacity * sizeof(Item));
        if (!tmp) { printf("Memory error!\n"); exit(1); }
        m->items = tmp;
    }
    m->items[m->count++] = it;
}

int findById(const Menu *m, int id) {
    for (int i = 0; i < m->count; i++)
        if (m->items[i].id == id) return i;
    return -1;
}

void saveMenu(const Menu *m) {
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp) { printf("Warning: could not save stock data.\n"); return; }
    for (int i = 0; i < m->count; i++) {
        const Item *p = &m->items[i];
        fprintf(fp, "%d,%s,%.2f,%d\n", p->id, p->name, p->price, p->stock);
    }
    fclose(fp);
}

void loadMenu(Menu *m) {
    FILE *fp = fopen(DATA_FILE, "r");
    if (fp) {
        Item it;
        while (fscanf(fp, "%d,%31[^,],%f,%d\n", &it.id, it.name, &it.price, &it.stock) == 4)
            addToMenu(m, it);
        fclose(fp);
    }
    if (m->count == 0) {  
      Item d[] = {
            { 1, "Samosa",              15.0f,  40},
            { 2, "French Fries",        60.0f,  35},
            { 3, "Peri Peri Fries",     80.0f,  30},
            { 4, "Veg Spring Roll",     50.0f,  25},
            { 5, "Veg Burger",          45.0f,  25},
            { 6, "Cheese Burger",       70.0f,  20},
            { 7, "Paneer Burger",       85.0f,  15},
            { 8, "Margherita Pizza",   150.0f,  15},
            { 9, "Veggie Pizza",       180.0f,  12},
            {10, "White Sauce Pasta",  110.0f,  18},
            {11, "Red Sauce Pasta",    100.0f,  18},
            {12, "Maggi",               40.0f,  30},
            {13, "Hakka Noodles",       90.0f,  20},
            {14, "Chai",                10.0f, 100},
            {15, "Hot Coffee",          30.0f,  60},
            {16, "Cold Coffee",         60.0f,  40},
            {17, "Chocolate Shake",     80.0f,  30},
            {18, "Fresh Lime Soda",     35.0f,  40},
            {19, "Cold Drink (Can)",    40.0f,  50},
            {20, "Mineral Water",       20.0f,  80}
        };
        int n = (int)(sizeof d / sizeof d[0]);
        for (int i = 0; i < n; i++) addToMenu(m, d[i]);
        saveMenu(m);
    }
}

int nextOrderNo(void) {
    int n = FIRST_ORDER;
    FILE *fp = fopen(ORDER_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%d", &n) != 1 || n < FIRST_ORDER) n = FIRST_ORDER;
        fclose(fp);
    }
    n++;
    fp = fopen(ORDER_FILE, "w");
    if (fp) { fprintf(fp, "%d\n", n); fclose(fp); }
    return n;
}

void logSale(int orderNo, float total, int itemsSold) {
    FILE *fp = fopen(SALES_FILE, "a");
    if (!fp) return;
    time_t now = time(NULL);
    char *ts = ctime(&now);
    ts[strcspn(ts, "\n")] = '\0';
    fprintf(fp, "%s | order #%d | items: %d | bill: %.2f\n", ts, orderNo, itemsSold, total);
    fclose(fp);
}

void showMenu(const Menu *m) {
    printf("\n================ TODAY'S MENU ================\n");
    printf("%-4s %-22s %-10s %-10s\n", "ID", "ITEM", "PRICE(Rs)", "AVAILABLE");
    printf("----------------------------------------------\n");
    for (int i = 0; i < m->count; i++) {
        const Item *p = &m->items[i];
        if (p->stock > 0)
            printf("%-4d %-22s %-10.2f %-10d\n", p->id, p->name, p->price, p->stock);
        else
            printf("%-4d %-22s %-10.2f %-10s\n", p->id, p->name, p->price, "SOLD OUT");
    }
    printf("----------------------------------------------\n");
}

int qtyInCart(const Cart *c, int idx) {
    for (int i = 0; i < c->count; i++)
        if (c->lines[i].idx == idx) return c->lines[i].qty;
    return 0;
}

void addToCart(Cart *c, const Menu *m) {
    printf("\n--- ADD TO CART (enter 0 to go back) ---\n");
    int id = readInt("Item ID: ");
    if (id == 0) return;

    int idx = findById(m, id);
    if (idx == -1) { printf("No such item. Check the menu (option 1).\n"); return; }

    const Item *p = &m->items[idx];
    int already = qtyInCart(c, idx);
    int left = p->stock - already;

    if (left <= 0) {
        printf("Sorry, %s is %s.\n", p->name,
               already > 0 ? "fully in your cart already" : "sold out");
        return;
    }

    printf("%s - Rs %.2f (%d available)\n", p->name, p->price, left);
    int qty = readInt("Quantity: ");
    if (qty <= 0)   { printf("Invalid quantity.\n"); return; }
    if (qty > left) { printf("Only %d available.\n", left); return; }

  for (int i = 0; i < c->count; i++) {
        if (c->lines[i].idx == idx) {
            c->lines[i].qty += qty;
            printf("Cart updated: %d x %s\n", c->lines[i].qty, p->name);
            return;
        }
    }
    if (c->count >= MAX_CART) { printf("Cart is full!\n"); return; }
    c->lines[c->count].idx = idx;
    c->lines[c->count].qty = qty;
    c->count++;
    printf("Added %d x %s to cart.\n", qty, p->name);
}

float cartTotal(const Cart *c, const Menu *m, int n) {
    if (n == 0) return 0.0f;
    const CartLine *l = &c->lines[n - 1];
    return l->qty * m->items[l->idx].price + cartTotal(c, m, n - 1);
}

float discountRate(float total) {
    if (total >= 500)      return 0.10f;
    else if (total >= 250) return 0.05f;
    return 0.0f;
}

float printBill(const Cart *c, const Menu *m, const char *title) {
    float sub  = cartTotal(c, m, c->count);
    float disc = sub * discountRate(sub);
    float gst  = (sub - disc) * GST_RATE;
    float net  = sub - disc + gst;

    printf("\n============== %s ==============\n", title);
    printf("%-3s %-20s %3s x %7s = %9s\n", "No", "Item", "Qty", "Price", "Amount");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < c->count; i++) {
        const Item *p = &m->items[c->lines[i].idx];
        printf("%-3d %-20s %3d x %7.2f = %9.2f\n", i + 1, p->name,
               c->lines[i].qty, p->price, c->lines[i].qty * p->price);
    }
    printf("----------------------------------------------------\n");
    printf("Subtotal : %10.2f\n", sub);
    printf("Discount : %10.2f\n", disc);
    printf("GST (5%%) : %10.2f\n", gst);
    printf("TOTAL    : %10.2f\n", net);
    printf("====================================================\n");
    return net;
}

void viewCart(const Cart *c, const Menu *m) {
    if (c->count == 0) { printf("\nYour cart is empty.\n"); return; }
    printBill(c, m, "YOUR CART");
}

void removeFromCart(Cart *c, const Menu *m) {
    if (c->count == 0) { printf("\nYour cart is empty.\n"); return; }
    viewCart(c, m);
    int no = readInt("Enter item No to remove (0 to go back): ");
    if (no == 0) return;
    if (no < 1 || no > c->count) { printf("Invalid number.\n"); return; }

    printf("Removed %s from cart.\n", m->items[c->lines[no - 1].idx].name);
    for (int i = no - 1; i < c->count - 1; i++)   
        c->lines[i] = c->lines[i + 1];
    c->count--;
}

void checkout(Cart *c, Menu *m) {
    if (c->count == 0) { printf("\nYour cart is empty. Add items first.\n"); return; }

    float net = printBill(c, m, "CONFIRM ORDER");
    char ans[8];
    readLine("Confirm and pay? (y/n): ", ans, sizeof ans);
    if (ans[0] != 'y' && ans[0] != 'Y') { printf("Order not placed. Cart kept.\n"); return; }

    int itemsSold = 0;
    for (int i = 0; i < c->count; i++) {
        m->items[c->lines[i].idx].stock -= c->lines[i].qty;
        itemsSold += c->lines[i].qty;
    }

    int orderNo = nextOrderNo();
    saveMenu(m);
    logSale(orderNo, net, itemsSold);
    c->count = 0;

    printf("\n****************************************\n");
    printf("      ORDER PLACED SUCCESSFULLY!\n");
    printf("      YOUR ORDER NUMBER : #%d\n", orderNo);
    printf("      Amount to pay     : Rs %.2f\n", net);
    printf("****************************************\n");
    printf("Please pay at the counter and show your order number.\n");
    printf("Thank you!\n");
}

int main(void) {
    Cart cart = { .count = 0 };

    initMenu(&menu);
    atexit(cleanup);
    loadMenu(&menu);

    printf("Welcome to the Campus Canteen!\n");

    int choice;
    do {
        printf("\n========== MAIN MENU ==========\n");
        printf("1. View menu\n");
        printf("2. Add item to cart\n");
        printf("3. View cart\n");
        printf("4. Remove item from cart\n");
        printf("5. Checkout\n");
        printf("0. Exit\n");
        printf("Cart items: %d\n", cart.count);
        choice = readInt("Choice: ");

        switch (choice) {
            case 1: showMenu(&menu);              break;
            case 2: addToCart(&cart, &menu);      break;
            case 3: viewCart(&cart, &menu);       break;
            case 4: removeFromCart(&cart, &menu); break;
            case 5: checkout(&cart, &menu);       break;
            case 0:
                if (cart.count > 0) printf("Cart discarded (not ordered).\n");
                printf("Thank you, visit again!\n");
                break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);

    return 0;
}
