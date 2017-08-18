# SimhashPhpExtension
Simhash algorithm written in PHP extension. In order to compare the similarity between Texts.

Demo:
```php
<?php
	$s = new simhash();

	// $str1 = '你 妈妈 喊 你 回家 吃饭 哦 回家 罗 回家 罗';
	// $str2 = '你 妈妈 叫 我 回家 吃饭 啦 回家 啦 回家 啦';
	$str1 = '听 下面 5 段 对话，每段 对话 后 有一个 小题，从 题中 所给的 A、B、C 三个 选项 中 选出 最佳 选项，并 标在 试卷 的 相应 位置，听完 每段 对话 后，你 都有 10 秒钟 的 时间 来 回答 有关 小题 和 阅读 下一小题。每段 对话 仅 读一遍。';
	// $str2 = '听 下面 8 段 对话，每段 对话 后 有三个 小题，从 题中 所出的 A、D、C 一个 选项 中 选出 最好 选项，并 写在 试卷 的 最前 位置，听完 每段 对话 前，你 都有 20 秒钟 的 时间 去 写下 有关 大题 和 查看 下一小题。这段 对话 仅 读两遍。';
	$str2 = '它 妈妈 叫 我 回家 吃饭 啦 回家 啦 回家 啦';

	$hash1 = $s->hash(explode(' ', $str1));
	$hash2 = $s->hash(explode(' ', $str2));

	$fingerPrint1 = $s->getBinary($hash1);
	$fingerPrint2 = $s->getBinary($hash2);
	$compare	  = $s->compare($fingerPrint1, $fingerPrint2);
	$hamming      = $s->hamming($compare);

	var_dump(sprintf("%064b", $fingerPrint1));
	echo "<br/>";
	var_dump(sprintf("%064b", $fingerPrint2));
	echo "<br/>";
	var_dump(sprintf("%064b", $compare));
	echo "<br/>";
	var_dump($hamming);
	echo "<br/>";
	echo ((1 - ($hamming/64))*100).'%';
?>
```
