// 送信するデータ
const data = {
  title: "tit-21",
};

async function postData() {
  try {
    const response = await fetch("http://localhost:8000/todos", {
      method: "POST", // メソッドを指定
      headers: {
        "Content-Type": "application/json", // JSONを送ることを伝える
      },
      body: JSON.stringify(data), // オブジェクトを文字列に変換
    });

    // 成功したかチェック（ステータスコード 200-299）
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    //const result = await response.json(); // レスポンスを解析
    const result = await response.text(); // レスポンスを解析
    console.log("成功:", result);
  } catch (error) {
    console.error("エラー発生:", error);
  }
}

postData();
