#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)

const char HTML[] PROGMEM = "text/html";

// http://stackoverflow.com/questions/3780511/reconnection-of-client-when-server-reboots-in-websocket
const char PAGE_TRACE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html><head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>xPablo.cz TRACE</title>
<style>
.ei {
  background: url('data:image/png;base64,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')  no-repeat #FBE3E4 5px 5px; padding: 1em; padding-left: 3.5em; border: 2px solid #FBC2C4; color: #D12F19;
}

html {
    display: table;
    margin: auto;
}

    body {
      background-color:#f9f9f9; color:#555555;
      font-family:Helvetica Neue, Helvetica, Arial, sans-serif;
      font-weight:400; font-size:85%;
    }

    h3 { font-weight:500; font-size:1.4em; }

    p { height:0.9em; }

    label {
      display:inline-block; width:10em;
      text-align:right; padding-right:0.4em;
    }

fieldset {
  padding-right:2em; padding-left:2em; display:inline-block;
  background-color:#efefef; border:solid 1px #dddddd;
  border-radius:0.3em; width:22em;
}

legend {
  font-size:1.15em; font-weight:500; background-color:#efefef;
  font-weight: bold;
  border-width:1px; border-style:solid; border-color:#dddddd;
  border-radius:0.3em; padding-left:0.5em; padding-right:0.5em;
}

</style>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    </head>
    <script>
      var connection;
      var connected = 0;

      function ping() {
        if (connected) {
          if (document.getElementById("live").checked)
            connection.send('mem');
          else
            connection.send('ping');
          setTimeout(ping, 5000);
        }
      }

      function connect() {
        connection = new WebSocket('ws://' + window.location.hostname + ':)====="
STRINGIZE(TRACE_WEBSOCKET) R"=====(/', ['arduino']);

        connection.onopen = function() {
          console.log('WS Connected');
          connected = 1;
          document.getElementById('errinfo').style = "display:none";
          ping(); // ping pro rychlejsi detekci ztraty spojeni
        };

        connection.onerror = function(error) {
          console.log('WS Error ', error);
          connected = 0;
        };

        connection.onmessage = function(e) {
          var d = JSON.parse(e.data);
          if (d.type == 'trace') {
            document.getElementById('trace').innerHTML = d.text;
          }
          if (d.type == 'info') {
            document.getElementById('reset').innerHTML = d.reset;
            document.getElementById('flash').innerHTML = d.flash;
            document.getElementById('ram').innerHTML = d.ram;
          }
          if (d.type == 'mem') {
            document.getElementById('ram').innerHTML = d.ram;
          }
        }

        connection.onclose = function() {
          console.log('WS Reconnecting timer start');
          setTimeout(function(){connect()}, 5000);
          document.getElementById('errinfo').style = "display:show";
          connected = 0;
        }
      }
    </script>
    <body onload="connect()">
    <h3>Informace</h3>
    <fieldset>
    Příčina resetu: <span id='reset'>???</span><br>
    Velikost flash: <span id='flash'>???</span> bytů<br>
    Volná RAM: <span id='ram'>???</span> bytů <input type="checkbox" id="live">Stále aktualizovat<br>
    </fieldset>
    <br />
    <h3>Stopař</h3>
    <div class='fixed' id=trace>
      <span style='color:black'> NAHRÁVÁM ...</span>
    </div>
    <br />
    <div class="ei" id="errinfo" style="display:none">Obnovuji spojení...</div>
    </body>
</html>
)=====";
