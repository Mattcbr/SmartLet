(function() {
	window.Main = {};
	Main.Page = (function() {
		var mosq = null;
		function Page() {
			var _this = this;
			mosq = new Mosquitto();

			$('#connect-button').click(function() {
				return _this.connect();
			});
			$('#disconnect-button').click(function() {
				return _this.disconnect();
			});
			$('#subscribe-button').click(function() {
				return _this.subscribe();
			});
			$('#unsubscribe-button').click(function() {
				return _this.unsubscribe();
			});
			
			
			$('#liga-output').click(function() {
				var payload = "L";  
				var TopicPublish = $('#pub-topic-text')[0].value;				
				mosq.publish(TopicPublish, payload, 0);
			});

			
			$('#desliga-output').click(function() {
				var payload = "D";  
				var TopicPublish = $('#pub-topic-text')[0].value;				
				mosq.publish(TopicPublish, payload, 0);
			});

			mosq.onconnect = function(rc){
				var p = document.createElement("p");
				var topic = $('#pub-subscribe-text')[0].value;
				p.innerHTML = "Conectado ao Broker!";
				$("#debug").append(p);
				mosq.subscribe(topic, 0);
				
			};
			mosq.ondisconnect = function(rc){
				var p = document.createElement("p");
				var url = "ws://iot.eclipse.org/ws";
				
				p.innerHTML = "A conexão com o broker foi perdida";
				$("#debug").append(p);				
				mosq.connect(url);
			};
			mosq.onmessage = function(topic, payload, qos){
				var p = document.createElement("p");
				var q = document.createElement("p");
				var r = document.createElement("p");
				//var msg = payload[1];
				var acao = payload[0];
				var ligado = payload[1];
				var volt = "";
				var i = 0;
				
				for(i = 2; i<payload.length; i++)
				{
					volt += payload[i];
				}
				
				//escreve o estado do output conforme informação recebida
				if (acao == 'L')
					p.innerHTML = "<center>Sistema Ativado (Cortando Corrente)</center>"
				else
					p.innerHTML = "<center>Sistema Desativado (Corrente Passando)</center>"
				
				//Novo
				if (ligado == 'N')
					q.innerHTML = "<center>Provavelmente Não</center>"
				else
					q.innerHTML = "<center> Provavelmente Sim</center>"
				
				r.innerHTML = volt
				
				$("#status_io").html(p);
				$("#ligado").html(q);
				$("#corrente").html(r);
				
			};
		}
		Page.prototype.connect = function(){
			var url = "ws://iot.eclipse.org/ws";
			mosq.connect(url);
		};
		Page.prototype.disconnect = function(){
			mosq.disconnect();
		};
		Page.prototype.subscribe = function(){
			var topic = $('#sub-topic-text')[0].value;
			mosq.subscribe(topic, 0);
		};
		Page.prototype.unsubscribe = function(){
			var topic = $('#sub-topic-text')[0].value;
			mosq.unsubscribe(topic);
		};
		
		return Page;
	})();
	$(function(){
		return Main.controller = new Main.Page;
	});
}).call(this);

