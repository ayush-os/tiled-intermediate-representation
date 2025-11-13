# tiled IR

### Untiled

```c
for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
        C[i, j] = A[i, j] + B[i, j];
    }
}
```

### Tiled (Tile Size $T$)

```c
for (int ii = 0; ii < M; ii += T) {
    for (int jj = 0; jj < N; jj += T) {
        for (int i = ii; i < min(ii + T, M); i++) {
            for (int j = jj; j < min(jj + T, N); j++) {
                C[i, j] = A[i, j] + B[i, j];
            }
        }
    }
}
```

---

### **Untiled** IR Tree

* **ROOT (Loop $L_i$)**
    * **Loop: $i$** ($0$ to $M$ step $1$)
        * **Body**
            * **Loop: $j$** ($0$ to $N$ step $1$)
                * **Body**
                    * **ASSIGN** ($C[i, j] = A[i, j] + B[i, j]$)
                        * **TARGET (LHS)**
                            * **STORE** (Tensor $C$, Indices $[i, j]$)
                        * **VALUE (RHS)**
                            * **ADD**
                                * **OPERAND 1**
                                    * **LOAD** (Tensor $A$, Indices $[i, j]$)
                                * **OPERAND 2**
                                    * **LOAD** (Tensor $B$, Indices $[i, j]$)

---

### **Tiled** IR Tree

* **ROOT**
    * **Loop: $\mathbf{ii}$** ($0$ to $M$ step $T$)
        * **Body**
            * **Loop: $\mathbf{jj}$** ($0$ to $N$ step $T$)
                * **Body**
                    * **Loop: $i$** ($ii$ to $\text{MIN}(ii + T, M)$ step $1$)
                        * **Body**
                            * **Loop: $j$** ($jj$ to $\text{MIN}(jj + T, N)$ step $1$)
                                * **Body**
                                    * **ASSIGN** ($C[i, j] = A[i, j] + B[i, j]$)
                                        * **TARGET (LHS)**
                                            * **STORE** (Tensor $C$, Indices $[i, j]$)
                                        * **VALUE (RHS)**
                                            * **ADD**
                                                * **OPERAND 1**
                                                    * **LOAD** (Tensor $A$, Indices $[i, j]$)
                                                * **OPERAND 2**
                                                    * **LOAD** (Tensor $B$, Indices $[i, j]$)

---