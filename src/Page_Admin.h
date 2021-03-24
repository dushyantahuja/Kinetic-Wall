//
//  HTML PAGE
//

const char PAGE_AdminMainPage[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<strong>Administration</strong>
<hr>
<a href="ON" style="width:250px" class="btn btn--m btn--blue" >Switch On</a><br>
<a href="OFF" style="width:250px" class="btn btn--m btn--blue" >Switch Off</a><br>
<a href="Increase" style="width:250px" class="btn btn--m btn--blue" >Increase Speed</a><br>
<a href="Decrease" style="width:250px" class="btn btn--m btn--blue" >Decrease Speed</a><br>
<a href="IncAcc" style="width:250px" class="btn btn--m btn--blue" >Increase Acceleration</a><br>
<a href="DecAcc" style="width:250px" class="btn btn--m btn--blue" >Decrease Acceleration</a><br>
<a href="update" style="width:250px" class="btn btn--m btn--blue" >Upload New Firmware</a><br>
<script>
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====";