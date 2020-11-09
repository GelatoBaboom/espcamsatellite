function Configs() {
    this.me = new Configs_CLASS();
    this.me.init();
    return this.me;
}
Configs.prototype.action = function () {
    //return this.click;
}
function Configs_CLASS() {
    return {
        btnResetEl: 'resetBtn',
        btnSendEl: 'sendBtn',
        init: function () {
            var thiscomp = this;
            thiscomp.activeResetDevice();
            thiscomp.bindSendTest();
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/getConfigsJson',
                processData: true,
                async: true,
                success: function (resp) {
                    $.each(resp, function (idx, cfg) {
                        var el = $('#' + cfg.key);
                        el.val(cfg.value);
                        el.change(function () {
                            thiscomp.updateField(cfg.key, el.val());
                        });

                    });
                }
            });
        },
        updateField: function (key, value) {
            var thiscomp = this;
            $('#' + thiscomp.btnResetEl).prop('disabled', true);
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/setConfig',
                processData: true,
                data: { k: key, v: value },
                async: true,
                success: function (resp) {
                    if (resp.result) {
                        thiscomp.message("Cambios guardados");
                        $('#' + thiscomp.btnResetEl).prop('disabled', false);
                    }
                    else {
                        setTimeout(function () { thiscomp.updateField(key, value); }, 2000);
                    }
                }
            });
        },
        activeResetDevice: function () {
            var thiscomp = this;
            $('#' + thiscomp.btnResetEl).click(function () {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/reboot',
                    processData: true,
                    async: true,
                    success: function (resp) {
                        thiscomp.message("Reiniciando en unos segundos..");
                    }
                });
                setTimeout(function () { window.location = "/"; }, 3000);
            });
        },
        bindSendTest: function () {
            var thiscomp = this;
            $('#' + thiscomp.btnSendEl).click(function () {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/sendtest',
                    processData: true,
                    async: true,
                    success: function (resp) {
                        if (resp.result) {
                            thiscomp.message("Prueba enviada!");
                        } else {
                            thiscomp.message("Error al enviar la prueba");
                        }
                    }
                });
            });
        },
        message: function (textMsg) {
            var el = $('#saveMsg');
            el.children().text(textMsg);
            el.animate({ top: '+=60' }).delay(3000).animate({ top: '-=60' });

        }

    }
}
$(document).ready(function () {
    var cfg = new Configs();

});
