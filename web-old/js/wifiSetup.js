var esp = esp || {};

esp.wifiSetup = (function() {
    var apScanUrl = '/api/apscan';
    var setupUrl = '/api/setup';
    var elements = { };
    var currentApList = [];

    function isApRow(el) {
      return esp.share.hasClass('ap-row', el);
    }

    document.addEventListener("DOMContentLoaded", function(event) { 
        elements.content = document.getElementById('content');
        elements.apTable = document.getElementById('wifi-aplist');
        elements.wifiForm = document.getElementById('wifi-setup-form');
        elements.refreshAps = document.getElementById('refresh-aps');
        elements.refreshAps.addEventListener('click', sendRefresh);
        
        elements.setAp = document.getElementById('set-ap');
        //elements.setAp.addEventListener('click', sendSetup);
        elements.wifiForm.addEventListener("submit", function (evt) {
            sendSetup();
            evt.preventDefault();
            return false;
        });

        elements.ssid = document.getElementById('ap-ssid');
        elements.password = document.getElementById('ap-password');
        elements.mqttHost = document.getElementById('mqtt-host');
        elements.mqttPort = document.getElementById('mqtt-port');
        elements.loading = document.getElementById('loading');

        document.body.addEventListener('click', function(evt) {
          var row = isApRow(evt.target) ? evt.target : esp.share.findAncestor(isApRow, evt.target);

          if(row) {
            rowClick(row);
          }
        });

        elements.loading.style.display = 'none';
        elements.content.style.display = '';
        
        getSetup();
    });

    var auth = {
        open: 0,
        wep: 1,
        wpaPsk: 2,
        wpa2Psk: 3,
        wpaWpa2Psk: 4,
        max: 5
    };
    
    function setMuiField(element, value) {
        element.value = value;
        var evt = document.createEvent("HTMLEvents");
        evt.initEvent("change", false, true);
        element.dispatchEvent(evt);
    }

    function anyTrueAnscestors(element, fn) {
      if(fn(element)) {
        return true;
      }
      
      return element.parentElement ?
        anyTrueAnscestors(element.parentElement, fn) :
        false;
    }
    
    function rowClick(row) {
      var isSelected = esp.share.hasClass('ap-selected', row);
      var tbody = elements.apTable.getElementsByTagName('tbody')[0];
      var rows = [].slice.call(tbody.children);

      rows.forEach(function(r) {
        esp.share.removeClass('ap-selected', r);
      });

      if(!isSelected) {
        esp.share.addClass('ap-selected', row);
        setMuiField(elements.ssid, row.getAttribute('ap-ssid'));
      }
    }

    function scanError() {
      clearApList();
      //TODO error...
    }

    function getSetup() {
      modalWaitPromise(esp.share.post(setupUrl, {type: 'get'}))
        //getTest()
          .then(function(data){
            setMuiField(elements.ssid, data.ssid);
            setMuiField(elements.mqttHost, data.mqttHost);
            setMuiField(elements.mqttPort, data.mqttPort);
          });
    }

    function sendSetup() {
      var payload = {
        type: 'set',
        setup: {
          ssid: elements.ssid.value,
          password: elements.password.value,
          mqttHost: elements.mqttHost.value,
          mqttPort: parseInt(elements.mqttPort.value),
        }
      };

      modalWaitPromise(esp.share.post(setupUrl, payload))
      //modalWaitPromise(setTest())
        .then(function _success(data) {
          if(data.error) {
            activateModalMessage('ESP Error: ' + data.error);
          } else {
            activateModalMessage('AP updated, Rebooting..');
          }
        }, function _fail(err){
          activateModalMessage('Fail: ' + err);
        });
    }

    function sendRefresh() {
      elements.refreshAps.disabled = true;
      clearApList();
      var loading = elements.loading.cloneNode(true);
      loading.style.display = '';
      elements.apTable.appendChild(crel('tbody',loading));

      var pollLoop = null;
      pollLoop = function() {
        setTimeout(function() {
          esp.share.post(apScanUrl, {aplist: 'list'})
          //testRefreshDone()
            .then(function(data) {
              if(data.status === 'SCANNING')  {
                pollLoop();
              } else if(data.status === 'SCAN ERROR') {
                scanError();
              } else if(data.status === 'FINISHED') {
                populateApList(data.apList || []);
              }
              elements.refreshAps.disabled = false;
            });
        }, 2000);
      };

      esp.share.post(apScanUrl, {aplist: 'refresh'})
      //testRefreshStart()
        .then(function(response) {
          if(response.status === 'SCANNING') {
            pollLoop();
          }
        });
    }

    function askForList() {
        return esp.share.post(url, {aplist: 'list'});
    }

    function clearApList(){
          elements.apTable.innerHTML = '';
    }

    function populateApList(apList) {
        clearApList();
        var rows = null;
        rows = crel('tbody', apList.map(function(ap, idx){
          return crel('tr', {'class': 'ap-row', 'ap-ssid': ap.ssid}, 
            crel('td', {'class': 'col-ssid'}, ap.ssid),
            crel('td', {'class': 'col-strength'}, ap.strength),
            crel('td', {'class': 'col-auth'}, ap.auth === 0 ? "Open" : "Secured"))
          }));
        elements.apTable.appendChild(rows); 
    }
 
    function modalWaitPromise(promise) {
      var options = {
        'keyboard': false, // teardown when <esc> key is pressed (default: true)
        'static': true, // maintain overlay when clicked (default: false)
        'onclose': function() {} // execute function when overlay is closed
      };
      var loading = elements.loading.cloneNode(true);
      loading.style.display = '';
      elements.apTable.appendChild(crel('tbody',loading));
      
      mui.overlay('on', options, crel('div', {'class': 'modalWait'}, loading));
      return new Promise(function(resolve, reject) {
        promise.then(function(data) {
          mui.overlay('off');
          resolve(data);
        }, function(err) {
          mui.overlay('off');
          reject(err);
        });
      });
    }
    function activateModalMessage(message) {
      var modalEl = crel('div', {'class': 'modalMain'},
        crel('div', {'class': 'modalMsg'}, message));
      // show modal
      mui.overlay('on', modalEl); 
    }

    return {
        
    };

    function testRefreshStart() {
      return new Promise(function(resolve,reject){
        resolve({"status":"SCANNING","apList":[]});
      });
    }

    function testRefreshDone() {
      return new Promise(function(resolve,reject){
        resolve({});  
      });
    }

    function setTest() {
      return new Promise(function(resolve, reject) {
        setTimeout(function() {
          resolve({});
        }, 2000);
        
      });
    }
    function getTest() {
      return new Promise(function(resolve, reject) {
        resolve({});
      });
    }
}());