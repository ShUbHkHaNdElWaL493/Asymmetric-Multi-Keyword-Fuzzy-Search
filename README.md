# Asymmetric Multi-Keyword Fuzzy Search

**Author:** Shubh Khandelwal (CS22B1090)

---

## Overview
This project implements an **Asymmetric Multi-Keyword Fuzzy Search** scheme. It provides a secure, tunable engine for indexing document corpora and evaluating fuzzy search queries over encrypted data. Utilizing mathematical structures such as Bloom Filters, matrix operations, and secret key generation, the project securely maps document keywords into searchable indices and dynamically retrieves top-ranked matches.

The system is equipped with a high-performance **C++ backend** powered by the Crow microframework and a responsive, zero-build-step **React frontend**.

---

## Features

- **Secure Mathematical Indexing:** Leverages Bloom Filters and invertible matrices to securely encode query and index vectors.
- **Configurable Parameters:** Real-time configuration of search scheme parameters (L - LSH Functions, M - Security Parameter / Bit-array length).
- **Batch Document Ingestion:** Supports quick drag-and-drop `.csv` uploads via the frontend for indexing large sets of documents.
- **Real-Time Progress Tracking:** Thread-safe asynchronous tracking for the indexing progress of large datasets.
- **Fuzzy Keyword Matching:** Ranks documents optimally based on fuzzy vector matching scores (dot products of encoded arrays).
- **Simulation:** Web interface built with pure React 18, utilizing virtualized lists (`react-window`) to smoothly handle large datasets without browser lag.

---

## Cryptography Scheme

This project employs a variant of **Asymmetric Scalar-Product Preserving Encryption (ASPE)** combined with **Locality-Sensitive Hashing (LSH)** and Bloom Filters to enable secure, fuzzy multi-keyword searches without decrypting the underlying data.

### 1. Secret Key Generation

The secret key $SK$ consists of a randomly generated binary splitting vector $S \in \{0, 1\}^m$ and two randomly generated invertible matrices $M_1, M_2 \in \mathbb{R}^{m \times m}$.

### 2. Feature Extraction (Bloom Filter & LSH)

Before encryption, document keywords are broken down into bigrams and hashed into a unified numerical vector of length $m$. This vector is constructed using Locality-Sensitive Hashing principles, allowing "fuzzy" matching (where slightly misspelled or closely related keywords still yield a high similarity score). Let this plaintext index vector be $I$.

### 3. Index Encryption

When a document is indexed, its vector $I$ is split into two vectors, $I_1$ and $I_2$, based on the binary vector $S$. For each dimension $i$:

* If $S[i] = 1$: $I_1[i] = I_2[i] = I[i]$
* If $S[i] = 0$: $I_1[i] = 0.5 I[i] + r$ and $I_2[i] = 0.5 I[i] - r$ (where $r$ is a random number)

The split vectors are then encrypted using the transposes of the secret matrices to create the final secure index $E_I$:


$$E_I = (M_1^T I_1, M_2^T I_2)$$

### 4. Query Encryption (Trapdoor Generation)

When a user searches for a query vector $Q$, it is multiplied by a random positive scalar $\gamma$ to obscure exact term frequencies. It is then split into $Q_1$ and $Q_2$ using the *reverse* logic of the index generation:

* If $S[i] = 0$: $Q_1[i] = Q_2[i] = Q[i]$
* If $S[i] = 1$: $Q_1[i] = 0.5 Q[i] + r$ and $Q_2[i] = 0.5 Q[i] - r$

The query vectors are encrypted using the *inverses* of the secret matrices to create the secure trapdoor $E_Q$:


$$E_Q = (M_1^{-1} Q_1, M_2^{-1} Q_2)$$

### 5. Secure Matching

The server ranks documents by calculating the inner product between the trapdoor $E_Q$ and the secure index $E_I$. Because of how the matrices are arranged, the secret matrices mathematically cancel out during the dot product:


$$E_Q \cdot E_I = (M_1^{-1} Q_1) \cdot (M_1^T I_1) + (M_2^{-1} Q_2) \cdot (M_2^T I_2)$$

$$E_Q \cdot E_I = Q_1 \cdot I_1 + Q_2 \cdot I_2 = \gamma (Q \cdot I)$$

The resulting score is strictly proportional to the original dot product $Q \cdot I$. This mechanism preserves the exact semantic ranking of the fuzzy search without ever exposing the plaintext query $Q$, the index $I$, or the secret key.

---

## Simulation

### Prerequisites

* `Docker`

### Image creation

You can create a `Docker` image for the application from the root directory:

```bash
docker build -t fyp2026 .
```

### Running the container

Create and run a container from the `Docker` image:

```bash
docker run --rm -p 3000:3000 fyp2026
```

The server will bind to `http://localhost:3000/`.

### Accessing the simulation

Open your web browser and navigate to `http://localhost:3000/`.

1. **Configure:** Set the `L` and `M` parameters.
2. **Index:** Upload a CSV containing your dataset (format: `docId,docName,keyword1;keyword2`).
3. **Search:** Query your indexed data using appropriate query terms.