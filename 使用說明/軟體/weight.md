# 程式功能大綱

## 1. 檔案路徑與結構

- **檔案名稱**：`WeightPage`
- **功能模組**：
  - `@/components/leftMenu/LeftMenu`: 顯示左側選單。
  - `@/components/FinanceChart`: 顯示體重變化數據圖表。
  - `@/components/AddWeight`: 新增體重數據功能。
  - `@/lib/client`: Prisma 資料庫客戶端。
  - `@clerk/nextjs/server`: 處理用戶身份驗證。
  - `next/navigation`: 提供錯誤頁面導航功能。

---

## 2. 主要功能大綱

### 2.1 資料獲取與處理

- **從 URL 參數獲取 `username`**
  - 透過 `params.username` 提取使用者名稱。
- **從資料庫查詢使用者資訊**

  - 使用 Prisma 的 `findFirst` 方法查詢對應使用者。
  - `include._count` 計算使用者的體重記錄數量。

- **判斷使用者是否存在**
  - 若無法找到對應使用者，則導向 `notFound()`。

---

### 2.2 驗證使用者身份

- 使用 Clerk 的 `auth()` 方法，獲取當前用戶的 `userId`。

---

### 2.3 組件呈現與 UI 結構

- **左側選單**

  - 使用 `LeftMenu` 組件，呈現類型為 "profile" 的選單。

- **主內容區**
  1. **標題**
     - 若有 `name` 和 `surname`，顯示其全名，否則顯示 `username`。
     - 標題格式：`{用戶名稱}的體重變化`。
  2. **新增體重區塊**
     - 使用 `AddWeight` 組件實現新增體重數據功能。
  3. **體重變化圖表**
     - 使用 `FinanceChart` 組件顯示體重變化數據。

---

## 3. UI 布局

- 外層結構：`div` 使用 `flex` 佈局分為三部分：
  - **左側欄**：僅在 `xl` 尺寸以上顯示，占寬度 20%。
  - **主內容**：`lg` 螢幕占比 70%，`xl` 螢幕占比 60%。
  - **右側欄**：預留區域，暫未實現功能。

---

## 4. 需要注意的細節

- **Clerk 身份驗證**

  - 確保在 Pages Router 中正確使用 `getAuth()`。

- **Prisma 查詢**

  - 保證資料庫內部欄位名稱和查詢條件一致，避免錯誤。

- **UI 相容性**

  - 確認組件（如 `FinanceChart` 和 `AddWeight`）與布局和邏輯相容。

- **多語言處理**
  - 檢查頁面中文拼接方式，確保一致性。

---

## 5. 未來擴展建議

1. **優化使用者查詢**
   - 增加更多關聯數據（如體重記錄詳細信息），豐富顯示內容。
2. **改進錯誤處理**
   - 提供友好的錯誤提示頁面。
3. **增加使用者互動**
   - 例如加入圖表的交互功能。
4. **提升響應式設計**
   - 優化小螢幕設備上的顯示效果。
