DROP TABLE IF EXISTS benchmark_measurements CASCADE;
DROP TABLE IF EXISTS benchmark_runs CASCADE;
DROP TABLE IF EXISTS search_result_snapshots CASCADE;
DROP TABLE IF EXISTS search_history CASCADE;
DROP TABLE IF EXISTS document_terms CASCADE;
DROP TABLE IF EXISTS terms CASCADE;
DROP TABLE IF EXISTS documents CASCADE;

CREATE TABLE documents (
    document_id  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title        TEXT NOT NULL,
    body         TEXT NOT NULL,
    category     VARCHAR(100),
    source       TEXT,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_documents_category    ON documents (category);
CREATE INDEX idx_documents_created_at  ON documents (created_at DESC);

CREATE TABLE terms (
    term_id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    normalized_term    TEXT NOT NULL UNIQUE,
    document_frequency BIGINT NOT NULL DEFAULT 0
        CONSTRAINT chk_doc_freq_non_negative CHECK (document_frequency >= 0)
);

CREATE TABLE document_terms (
    document_id    BIGINT NOT NULL
        REFERENCES documents (document_id) ON DELETE CASCADE,
    term_id        BIGINT NOT NULL
        REFERENCES terms (term_id) ON DELETE CASCADE,
    term_frequency INTEGER NOT NULL
        CONSTRAINT chk_term_freq_positive CHECK (term_frequency > 0),
    PRIMARY KEY (document_id, term_id)
);

CREATE INDEX idx_document_terms_term_id ON document_terms (term_id);

CREATE TABLE search_history (
    search_id       BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    query_text      TEXT NOT NULL,
    query_type      VARCHAR(30) NOT NULL
        CONSTRAINT chk_query_type
            CHECK (query_type IN ('substring', 'autocomplete', 'range')),
    requested_limit INTEGER NOT NULL DEFAULT 10
        CONSTRAINT chk_limit_positive CHECK (requested_limit > 0),
    result_count    INTEGER NOT NULL DEFAULT 0
        CONSTRAINT chk_result_non_negative CHECK (result_count >= 0),
    elapsed_us      BIGINT
        CONSTRAINT chk_elapsed_non_negative CHECK (elapsed_us >= 0),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE search_result_snapshots (
    snapshot_id  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    search_id    BIGINT NOT NULL
        REFERENCES search_history (search_id) ON DELETE CASCADE,
    document_id  BIGINT NOT NULL
        REFERENCES documents (document_id) ON DELETE CASCADE,
    rank_position INTEGER NOT NULL CONSTRAINT chk_rank_pos CHECK (rank_position >= 1),
    score        DOUBLE PRECISION NOT NULL,
    match_count  INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX idx_snapshots_search_id ON search_result_snapshots (search_id);

CREATE TABLE benchmark_runs (
    benchmark_run_id  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    dataset_name      TEXT NOT NULL,
    dataset_size      BIGINT NOT NULL
        CONSTRAINT chk_dataset_size_non_negative CHECK (dataset_size >= 0),
    algorithm_name    VARCHAR(100) NOT NULL,
    operation_name    VARCHAR(100) NOT NULL,
    configuration     JSONB,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE benchmark_measurements (
    measurement_id    BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    benchmark_run_id  BIGINT NOT NULL
        REFERENCES benchmark_runs (benchmark_run_id) ON DELETE CASCADE,
    iteration_number  INTEGER NOT NULL
        CONSTRAINT chk_iter_positive CHECK (iteration_number > 0),
    elapsed_us        BIGINT NOT NULL
        CONSTRAINT chk_meas_elapsed CHECK (elapsed_us >= 0),
    result_count      BIGINT,
    memory_bytes      BIGINT
        CONSTRAINT chk_mem_non_negative CHECK (memory_bytes >= 0)
);

CREATE INDEX idx_measurements_run_id ON benchmark_measurements (benchmark_run_id);
