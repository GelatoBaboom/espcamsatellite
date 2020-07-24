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
                async: false,
                success: function (resp) {
                    $.each(resp, function (idx, cfg) {
                        var el = $('#' + cfg.key);
                        el.val(cfg.value);
                    });
                }
            });
        }
    }
}
$(document).ready(function () {
    var cfg = new Configs();
});
