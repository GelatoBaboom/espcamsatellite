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
        init: function () {
            var thiscomp = this;
            thiscomp.activeResetDevice();
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
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/getRTCConfigsJson',
                processData: true,
                async: true,
                success: function (resp) {
                    $.each(resp, function (idx, cfg) {
                        var el = $('#' + cfg.key);
                        el.val(cfg.value);
                        el.change(function () {
                            thiscomp.updateRTC(cfg.key, el.val());
                        });

                    });
                }
            });
        },
        updateRTC: function (key, value) {
            var thiscomp = this;
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/setRTCConfig',
                processData: true,
                data: { k: key, v: value },
                async: true,
                success: function (resp) {
                    thiscomp.message("Cambios guardados");
                }
            });
        },
        updateField: function (key, value) {
            var thiscomp = this;
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/setConfig',
                processData: true,
                data: { k: key, v: value },
                async: true,
                success: function (resp) {
                    thiscomp.message("Cambios guardados");
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
