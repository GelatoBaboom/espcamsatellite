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
        init: function () {
            var thiscomp = this;
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
                            console.log(this.value);
                            console.log(el.val());
                            thiscomp.updateField(cfg.key, el.val());
                        });

                    });
                }
            });
        },
        updateField: function (key, value) {
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/setConfig',
                processData: true,
                data: { k: key, v: value },
                async: true,
                success: function (resp) { }
            });
        }
    }
}
$(document).ready(function () {
    var cfg = new Configs();
});
