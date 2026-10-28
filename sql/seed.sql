INSERT INTO documents (title, body, category, source) VALUES
('Introduction to Algorithms',
 'An algorithm is a step-by-step procedure for solving a problem. Common algorithms include sorting and searching.',
 'algorithms', 'sample_documents'),

('Binary Search Trees',
 'A binary search tree stores keys in sorted order. Each node has at most two children. Search, insert and delete all run in O(log n) average time.',
 'data-structures', 'sample_documents'),

('AVL Trees and Balancing',
 'An AVL tree is a self-balancing binary search tree. The balance factor of every node is maintained between -1 and 1 using rotations.',
 'data-structures', 'sample_documents'),

('Suffix Arrays for Pattern Matching',
 'A suffix array contains sorted suffixes of a string. Binary search over the array enables pattern matching in O(P log N) time.',
 'algorithms', 'sample_documents'),

('Trie Data Structure',
 'A trie is a tree-like structure for storing strings. It supports fast prefix queries and is commonly used for autocomplete systems.',
 'data-structures', 'sample_documents'),

('Heaps and Priority Queues',
 'A max-heap is a complete binary tree where every parent key is greater than its children. It supports extract-max in O(log n) time.',
 'data-structures', 'sample_documents'),

('Database Indexing',
 'Database indexes accelerate queries by maintaining sorted structures over column values. B-trees and hash indexes are common choices.',
 'databases', 'sample_documents'),

('Text Search Engines',
 'A full-text search engine builds inverted indexes mapping terms to documents. Scoring models such as TF-IDF rank results by relevance.',
 'search', 'sample_documents'),

('Graph Algorithms',
 'Graph algorithms traverse edges and vertices to solve problems. Breadth-first search and Dijkstra are widely used for path finding.',
 'algorithms', 'sample_documents'),

('Dynamic Programming',
 'Dynamic programming solves problems by breaking them into overlapping subproblems and caching intermediate results. Memoization is a common technique.',
 'algorithms', 'sample_documents');
