// about.html file in raw data format for PROGMEM
//
const char about_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
 <head>
  <title>About ESP-radio</title>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ESP Radio</a></li>
   <li><a class="pull-left" href="/index.html">控制</a></li>
   <li><a class="pull-left" href="/config.html">配置</a></li>
   <li><a class="pull-left active" href="/about.html">关于</a></li>
  </ul>
  <br><br><br>
  <center>
   <h1>** ESP Radio **</h1>
  </center>
	<p>Esp Radio -- 基于ESP8266的Webradio接收器，1.8英寸彩色显示屏和VS1053 MP3模块。<br>
	该项目记录在<a target="blank" href="https://github.com/Edzelf/Esp-radio">Github</a>.</p>
	<p>Author: Ed Smallenburg<br>
	页面设计: <a target="blank" href="http://www.sanderjochems.nl/">Sander Jochems</a><br>
	App (Android): <a target="blank" href="https://play.google.com/store/apps/details?id=com.thunkable.android.sander542jochems.ESP_Radio">Sander Jochems</a><br>
	日期: January 2017</p>
  <script type="text/javascript">
    var stylesheet = document.createElement('link') ;
    stylesheet.href = 'radio.css' ;
    stylesheet.rel = 'stylesheet' ;
    stylesheet.type = 'text/css' ;
    document.getElementsByTagName('head')[0].appendChild(stylesheet) ;
  </script>
 </body>
</html>
)=====" ;
