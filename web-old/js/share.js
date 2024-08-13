var esp = esp || {};
esp.share = (function() {
    
    return {
        hasClass: function(className, element){
            return (" " + element.className + " ").replace(/[\n\t]/g, " ").indexOf(" " + className + " ") > -1;
        },
        addClass: function(className, element) {
            element.className += " " + className;
        },
        removeClass: function(className, element) {
            return element.className = element.className.replace(new RegExp('(?:^|\\s)'+ className + '(?:\\s|$)'), '');
        },  
        findAncestor: function(matchFn, element) {
            if(element.parentElement) {
                var parent = element.parentElement;
                if(matchFn(parent)) {
                    return parent;
                }
                return this.findAncestor(matchFn, parent);
            }
            return null;
        },
        post: function(url, data) {
            
            return new Promise(function(resolve, reject){
                var http = new XMLHttpRequest();
                http.open("POST", url, true);

                //Send the proper header information along with the request
                http.setRequestHeader("Content-type", "application/javascript");

                http.onreadystatechange = function() {
                    if(http.readyState == 4 && http.status == 200) {
                        resolve(JSON.parse(http.responseText));
                    }
                    else if(http.status != 200) {
                        reject("Invalid HTTP status: " + http.status    );
                    }
                };
                http.send(JSON.stringify(data));
            });
        }
    };
}
());