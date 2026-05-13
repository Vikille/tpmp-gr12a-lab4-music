-- =============================================
-- БАЗА ДАННЫХ «МУЗЫКАЛЬНЫЙ САЛОН» (SQLite)
-- =============================================

PRAGMA foreign_keys = ON;

-- 1. Удаление таблиц (если существуют) для чистого импорта
DROP TABLE IF EXISTS CD_PERIOD_STATS;
DROP TABLE IF EXISTS OPERATIONS;
DROP TABLE IF EXISTS MUSIC_TRACKS;
DROP TABLE IF EXISTS CD_DISCS;

-- --------------------------------------------------
-- 2. Создание таблиц
-- --------------------------------------------------

-- Таблица компакт-дисков
CREATE TABLE CD_DISCS (
    cd_code TEXT PRIMARY KEY,                  -- код компакта
    manufacture_date DATE NOT NULL,            -- дата изготовления
    producer_company TEXT NOT NULL,            -- компания-производитель
    price REAL NOT NULL CHECK (price > 0)      -- цена > 0
);

-- Таблица музыкальных произведений
CREATE TABLE MUSIC_TRACKS (
    track_id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,                       -- название
    author TEXT NOT NULL,                      -- автор
    performer TEXT NOT NULL,                   -- исполнитель
    cd_code TEXT NOT NULL,                     -- код компакта
    FOREIGN KEY (cd_code) REFERENCES CD_DISCS(cd_code)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- Таблица операций (поступление / продажа)
CREATE TABLE OPERATIONS (
    op_id INTEGER PRIMARY KEY AUTOINCREMENT,
    operation_date DATE NOT NULL,              -- дата операции
    operation_type TEXT NOT NULL CHECK (operation_type IN ('I', 'S')),
                                               -- 'I' – поступление, 'S' – продажа
    cd_code TEXT NOT NULL,                     -- код компакта
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    FOREIGN KEY (cd_code) REFERENCES CD_DISCS(cd_code)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- Таблица для хранения отчётов по периодам (п.5 задания)
CREATE TABLE CD_PERIOD_STATS (
    cd_code TEXT NOT NULL,
    period_start DATE NOT NULL,
    period_end DATE NOT NULL,
    total_in INTEGER NOT NULL DEFAULT 0,       -- поступило за период
    total_out INTEGER NOT NULL DEFAULT 0,      -- продано за период
    PRIMARY KEY (cd_code, period_start, period_end)
);

-- --------------------------------------------------
-- 3. Триггер для запрета продажи при нехватке остатка (п.4)
-- --------------------------------------------------
CREATE TRIGGER check_sale_stock
BEFORE INSERT ON OPERATIONS
FOR EACH ROW
WHEN NEW.operation_type = 'S'
BEGIN
    SELECT RAISE(ABORT, 'Продажа невозможна: недостаточно поступивших компактов')
    WHERE (
        COALESCE((SELECT SUM(quantity) FROM OPERATIONS
                  WHERE cd_code = NEW.cd_code AND operation_type = 'I'), 0)
        -
        COALESCE((SELECT SUM(quantity) FROM OPERATIONS
                  WHERE cd_code = NEW.cd_code AND operation_type = 'S'), 0)
    ) < NEW.quantity;
END;

-- --------------------------------------------------
-- 4. Представления для покупателей (помечены * в задании)
-- --------------------------------------------------

-- 4.1 Компакт, купленный максимальное количество раз – полные сведения
CREATE VIEW CUSTOMER_BESTSELLER AS
WITH sales_total AS (
    SELECT cd_code, SUM(quantity) AS total_qty
    FROM OPERATIONS
    WHERE operation_type = 'S'
    GROUP BY cd_code
),
max_sale AS (
    SELECT cd_code
    FROM sales_total
    WHERE total_qty = (SELECT MAX(total_qty) FROM sales_total)
    LIMIT 1
)
SELECT
    d.*,
    t.title, t.author, t.performer
FROM CD_DISCS d
JOIN max_sale m ON d.cd_code = m.cd_code
LEFT JOIN MUSIC_TRACKS t ON d.cd_code = t.cd_code;

-- 4.2 Самый популярный исполнитель – количество проданных дисков с его треками
CREATE VIEW CUSTOMER_TOP_PERFORMER AS
WITH performer_sales AS (
    SELECT
        t.performer,
        SUM(o.quantity) AS total_sold
    FROM OPERATIONS o
    JOIN MUSIC_TRACKS t ON o.cd_code = t.cd_code
    WHERE o.operation_type = 'S'
    GROUP BY t.performer
)
SELECT performer, total_sold
FROM performer_sales
WHERE total_sold = (SELECT MAX(total_sold) FROM performer_sales);

-- --------------------------------------------------
-- 5. ЗАПОЛНЕНИЕ ТАБЛИЦ ТЕСТОВЫМИ ДАННЫМИ
-- --------------------------------------------------

-- 5.1 Компакт-диски
INSERT INTO CD_DISCS (cd_code, manufacture_date, producer_company, price) VALUES
('CD-001', '2025-01-15', 'Melody Inc.', 12.99),
('CD-002', '2025-02-10', 'SoundWave', 9.99),
('CD-003', '2025-03-05', 'Vinyl Records', 15.50),
('CD-004', '2025-03-20', 'Melody Inc.', 10.00);

-- 5.2 Музыкальные треки
INSERT INTO MUSIC_TRACKS (title, author, performer, cd_code) VALUES
('Moonlight', 'Smith', 'John Band', 'CD-001'),
('Sunrise', 'Smith', 'John Band', 'CD-001'),
('Dreamer', 'Smith', 'Elena Duo', 'CD-001'),
('Storm', 'Black', 'Rockers', 'CD-002'),
('Calm', 'Black', 'Rockers', 'CD-002'),
('Fire', 'Black', 'Rockers', 'CD-002'),
('Ocean', 'White', 'Elena Duo', 'CD-003'),
('River', 'White', 'Elena Duo', 'CD-003'),
('Cloud', 'Smith', 'John Band', 'CD-004'),
('Star', 'Smith', 'John Band', 'CD-004');

-- 5.3 Операции поступления и продаж
-- Поступления
INSERT INTO OPERATIONS (operation_date, operation_type, cd_code, quantity) VALUES
('2025-02-01', 'I', 'CD-001', 100),
('2025-02-15', 'I', 'CD-002', 80),
('2025-03-01', 'I', 'CD-003', 50),
('2025-03-10', 'I', 'CD-004', 120);

-- Продажи
INSERT INTO OPERATIONS (operation_date, operation_type, cd_code, quantity) VALUES
('2025-02-10', 'S', 'CD-001', 5),
('2025-02-20', 'S', 'CD-001', 10),
('2025-02-28', 'S', 'CD-002', 20),
('2025-03-05', 'S', 'CD-003', 5),
('2025-03-12', 'S', 'CD-004', 15),
('2025-03-15', 'S', 'CD-001', 30),
('2025-03-18', 'S', 'CD-002', 25),
('2025-03-20', 'S', 'CD-003', 10);

-- --------------------------------------------------
-- 6. ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ ЗАПРОСОВ (можно раскомментировать)
-- --------------------------------------------------

/*
-- 6.1 По всем компактам: количество проданных и оставшихся (по убыванию остатка)
SELECT 
    d.cd_code,
    COALESCE(sold.total_sold, 0) AS sold_quantity,
    COALESCE(incomm.total_in, 0) - COALESCE(sold.total_sold, 0) AS remaining_quantity
FROM CD_DISCS d
LEFT JOIN (
    SELECT cd_code, SUM(quantity) AS total_in
    FROM OPERATIONS
    WHERE operation_type = 'I'
    GROUP BY cd_code
) incomm ON d.cd_code = incomm.cd_code
LEFT JOIN (
    SELECT cd_code, SUM(quantity) AS total_sold
    FROM OPERATIONS
    WHERE operation_type = 'S'
    GROUP BY cd_code
) sold ON d.cd_code = sold.cd_code
ORDER BY remaining_quantity DESC;

-- 6.2 По указанному компакту ('CD-001') за период (01.02.2025 – 15.03.2025)
SELECT 
    o.cd_code,
    SUM(o.quantity) AS total_sold,
    SUM(o.quantity * d.price) AS total_revenue
FROM OPERATIONS o
JOIN CD_DISCS d ON o.cd_code = d.cd_code
WHERE o.operation_type = 'S'
  AND o.cd_code = 'CD-001'
  AND o.operation_date BETWEEN '2025-02-01' AND '2025-03-15'
GROUP BY o.cd_code;

-- 6.3 По каждому автору: количество проданных дисков и выручка
SELECT 
    t.author,
    SUM(o.quantity) AS total_sold,
    SUM(o.quantity * d.price) AS total_revenue
FROM OPERATIONS o
JOIN MUSIC_TRACKS t ON o.cd_code = t.cd_code
JOIN CD_DISCS d ON o.cd_code = d.cd_code
WHERE o.operation_type = 'S'
GROUP BY t.author
ORDER BY total_revenue DESC;

-- 6.4 Заполнение таблицы CD_PERIOD_STATS за период (заменить даты при необходимости)
DELETE FROM CD_PERIOD_STATS WHERE period_start = '2025-02-01' AND period_end = '2025-03-20';
INSERT INTO CD_PERIOD_STATS (cd_code, period_start, period_end, total_in, total_out)
SELECT
    cd_code,
    '2025-02-01',
    '2025-03-20',
    COALESCE(SUM(CASE WHEN operation_type = 'I' THEN quantity ELSE 0 END), 0),
    COALESCE(SUM(CASE WHEN operation_type = 'S' THEN quantity ELSE 0 END), 0)
FROM OPERATIONS
WHERE operation_date BETWEEN '2025-02-01' AND '2025-03-20'
GROUP BY cd_code;

-- 6.5 Результаты продажи компакта 'CD-001' за период (детально)
SELECT
    o.operation_date,
    o.quantity,
    o.quantity * d.price AS revenue
FROM OPERATIONS o
JOIN CD_DISCS d ON o.cd_code = d.cd_code
WHERE o.operation_type = 'S'
  AND o.cd_code = 'CD-001'
  AND o.operation_date BETWEEN '2025-02-01' AND '2025-03-20'
ORDER BY o.operation_date;
*/
